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


// ****PENDIENTE********PENDIENTE********PENDIENTE****
// hacer el codigo funcionar con los inputs reales
// hacer que el umbral sea un parametro que se pase por consola (listo)
// hacer que la carpeta de genomas sea un parametro que se pase por consola(listo)
// verificar que se cumplan los requisitos
//verificar si esta bien hecho mutex y cv o rehacerlo
// si logran q el jesus les revise la wea joya 100% real no fake no scam no cap no virus 2020 1 link mega mediafire (todo esto lo sugirio copilot y me parecio el epitome  de la comedia)
// informe
// ****PENDIENTE********PENDIENTE********PENDIENTE****

using namespace std;

string carpeta_genomas; // donde estaran los genomas a procesar
mutex queueMutex;// mutex para la cola de genomas aceptados
condition_variable cv; // variable de condicion para la cola de genomas aceptados
queue<string> genomasAceptados; // esta es la queue que el problema dice q hay q usar pa meter los q cumplen con el umbral
float umbral; // Umbral para el contenido GC 
bool finalizado = false;

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
}   // si esta vacio, devuelve cero. Si no lo esta, saca promedio (esto evita que se divida por 0)

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

void procesar_genoma(const string& genoma_file, float umbral) { //procesa un genoma y lo mete a la cola si cumple con el umbral
    float promedio = calcular_promedio_GC(genoma_file); //calcula el promedio de GC del genoma con la funcion que ya esta definida arriba
    if (promedio >= umbral) { //si el promedio es mayor o igual al umbral, lo mete a la cola
        lock_guard<mutex> lock(queueMutex);// lock para la cola de genomas aceptados
        genomasAceptados.push(genoma_file); //mete el genoma a la cola
        cv.notify_one();// notifica a la variable de condicion que hay un genoma en la cola
    } //si no cumple con el umbral, no hace nada
    //importante: "umbral" es un parametro que pasar por consola ****PENDIENTE****
}

void consumir_genomas() {// funcion que consume los genomas de la cola
    unique_lock<mutex> lock(queueMutex);  // lock para la cola de genomas aceptados
    while (!finalizado || !genomasAceptados.empty()) { //mientras no se haya finalizado o la cola no este vacia
        cv.wait(lock, []{ return finalizado || !genomasAceptados.empty(); }); //espera a que haya un genoma en la cola o se finalice
        while (!genomasAceptados.empty()) { //mientras la cola no este vacia
            cout << "Genoma aceptado: " << genomasAceptados.front() << endl; //saca el genoma de la cola
            genomasAceptados.pop();// lo saca de la cola (pop) 
            //no se si esta wea esta bien xDDD ****PENDIENTE****
        }
    }
}

int main(int argc, char* argv[]) {
    vector<string> genomas = leer_genomas(carpeta_genomas);
    vector<thread> threads;

    if (argc < 3) { // verifica que se haya pasado al menos un argumento
        cerr << "Error: No se proporcionó un umbral." << endl;
        return 1;
    }

    umbral = atof(argv[1]); //convierte el argumento a float
    carpeta_genomas = argv[2]; //convierte el argumento a string

    thread consumidor(consumir_genomas);//crea el thread que consume los genomas

    for (const auto& genoma_file : genomas) { //crea un thread por cada genoma
        threads.emplace_back(procesar_genoma, genoma_file, umbral);//crea el thread que procesa el genoma
    }

    for (auto& t : threads) {
        t.join(); //espera a que todos los threads terminen
    }

    {
        lock_guard<mutex> lock(queueMutex); // lock para la cola de genomas aceptados
        finalizado = true; //finaliza el programa
    }
    cv.notify_one(); //notifica a la variable de condicion que se finalizo

    consumidor.join(); //espera a que el consumidor termine

    return 0;
}
