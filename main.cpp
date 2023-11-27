#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <chrono>

using namespace std;

sem_t sem;
string carpeta_genomas, tipo; // donde estaran los genomas a procesar
mutex queueMutex;// mutex para la cola de genomas aceptados
condition_variable cv; // variable de condicion para la cola de genomas aceptados
queue<string> genomasAceptados; // esta es la queue que el problema dice q hay q usar pa meter los q cumplen con el umbral
float umbral; // Umbral para el contenido GC
bool finalizado = false;
vector<thread> threads;

float calcular_promedio_GC(const string& genoma_file) { // calcula el promedio de GC de un genoma
    ifstream genoma_content(carpeta_genomas + genoma_file); // abrir el archivo del genoma (path y nombre del archivo)
    if (!genoma_content.is_open()) {
        cerr << "Error: No se pudo abrir el archivo " << genoma_file << endl;
        return -1.0; //por si el archivo deja de existir
    }

    string genoma; //hace una string para extraer lo que saca del archivo
    getline(genoma_content, genoma); //guarda el contenido en la string
    genoma_content.close(); //cierra el archivo

    int contador_GC = 0;
    for (char nucleotido : genoma) { //sumar G y C en el genoma pa despues sacar promedio
        if (nucleotido == 'G' || nucleotido == 'C') {
            contador_GC++;
        }
    }

    return genoma.empty() ? 0 : static_cast<float>(contador_GC) / genoma.size();
}

void consumir_genomas(string option){
    if (option == "mutex"){
        unique_lock<mutex> lock(queueMutex);
        while (!finalizado || !genomasAceptados.empty()) { 
            cv.wait(lock, [&]{ return finalizado || !genomasAceptados.empty(); });
            while (!genomasAceptados.empty()) {
                cout << "Genoma aceptado: " << genomasAceptados.front() << endl;
                genomasAceptados.pop();
            }
        }
    }
    else if (option == "sem"){
        while (!finalizado || !genomasAceptados.empty()) {
            sem_wait(&sem); // Espera a que el semáforo esté disponible
            while (!genomasAceptados.empty()) {
                cout << "Genoma aceptado: " << genomasAceptados.front() << endl;
                genomasAceptados.pop();
            }
            sem_post(&sem); // Libera el semáforo después de consumir genomas
        }
    }
}

void procesar_genoma(const string& genoma_file, float umbral, string option){
    if (option == "mutex"){
        float promedio = calcular_promedio_GC(genoma_file); //calcula el promedio de GC del genoma con la funcion que ya esta definida arriba
        if (promedio >= umbral) { //si el promedio es mayor o igual al umbral, lo mete a la cola
            lock_guard<mutex> lock(queueMutex);// lock para la cola de genomas aceptados
            genomasAceptados.push(genoma_file); //mete el genoma a la cola
            cv.notify_one();// notifica a la variable de condicion que hay un genoma en la cola
        }
    }
    else if (option == "sem"){
        float promedio = calcular_promedio_GC(genoma_file);
        if (promedio >= umbral) {
            sem_wait(&sem); // Espera a que el semáforo esté disponible
            genomasAceptados.push(genoma_file);
            sem_post(&sem); // Libera el semáforo después de agregar el genoma
        }
    }
}

void seleccionMutex(vector<string> genomas){
    thread consumidor(consumir_genomas,"mutex");//crea el thread que consume los genomas para mutex
    for (const auto& genoma_file : genomas) { //crea un thread por cada genoma
        threads.emplace_back(procesar_genoma, genoma_file, umbral,"mutex");//crea el thread que procesa el genoma
    }

    for (auto& t : threads) {
        t.join(); //espera a que todos los threads terminen
    }

    {
        lock_guard<mutex> lock(queueMutex);
        finalizado = true;
    }
    // Notificar al consumidor después de establecer finalizado en true
    cv.notify_one();
    consumidor.join();
}

void seleccionSemaforo(vector<string> genomas){
   thread consumidor(consumir_genomas, "sem");//crea el thread que consume los genomas para semaforo
    for (const auto& genoma_file : genomas) {
        threads.emplace_back(procesar_genoma, genoma_file, umbral,"sem");
    }

    for (auto& t : threads) {
        t.join();
    }

    sem_wait(&sem); // Espera a que el semáforo esté disponible antes de finalizar
    finalizado = true;
    sem_post(&sem); // Libera el semáforo después de establecer finalizado en true
    consumidor.join();
    sem_destroy(&sem); // Destruye el semáforo
}
vector<string> leer_genomas(const string& carpeta_genomas) { //toma todos los ficheros y retorna un vector con el nombre
                                                            //de todos los archivos en una carpeta
    vector<string> genomas;
    DIR* directorio = opendir(carpeta_genomas.c_str()); //abre directorio
    if (directorio == NULL) {
        cerr << "Error: No existe el directorio " << carpeta_genomas << endl;
        return genomas; //por si no existe
    }

    struct dirent* directorio_data; //estructura para leer el directorio
    while ((directorio_data = readdir(directorio)) != NULL) { //lee el directorio
        if (directorio_data->d_name[0] != '.') { //si no es un archivo oculto (empieza con .)
            genomas.push_back(directorio_data->d_name); //guarda el nombre del archivo en el vector
        }
    }

    closedir(directorio); //cierra el directorio
    return genomas; //retorna el vector con los nombres de los archivos
}

int main(int argc, char* argv[]) {
    if (argc < 3) { // verifica que se haya pasado al menos un argumento
            cerr << "Error: No se proporcionó un umbral." << endl;
            return 1;
    }

    umbral = atof(argv[1]); //convierte el argumento a float
    carpeta_genomas = argv[2]; //convierte el argumento a string
    tipo = argv[3]; //convierte el argumento a string
    if(carpeta_genomas[carpeta_genomas.length()-1]!='/')carpeta_genomas = carpeta_genomas + '/';
    vector<string> genomas = leer_genomas(carpeta_genomas);
    vector<thread> threads;
    sem_init(&sem, 0, 1); // Inicializa el semáforo
    thread consumidor;
    if(tipo == "mutex"){
        seleccionMutex(genomas);
    }
    else if(tipo == "sem"){
        seleccionSemaforo(genomas);
    }
    else{
        cout << "Error en el tercer argumento" << endl;
        return 0;
    }
    return 0;
}