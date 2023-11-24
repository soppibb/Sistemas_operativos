#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <fstream>
#include <thread>
#include <semaphore.h>
#include <queue>

using namespace std;

string carpeta_genomas;
sem_t sem; // semáforo para la cola de genomas aceptados
queue<string> genomasAceptados; // esta es la queue que almacena los genomas que cumplen con el umbral
float umbral;
bool finalizado = false;

float calcular_promedio_GC(const string& genoma_file) {
    ifstream genoma_content(carpeta_genomas + genoma_file);
    if (!genoma_content.is_open()) {
        cerr << "Error: No se pudo abrir el archivo " << genoma_file << endl;
        return -1.0;
    }

    string genoma;
    getline(genoma_content, genoma);
    genoma_content.close();

    int contador_GC = 0;
    for (char nucleotido : genoma) {
        if (nucleotido == 'G' || nucleotido == 'C') {
            contador_GC++;
        }
    }

    return genoma.empty() ? 0 : static_cast<float>(contador_GC) / genoma.size();
}

vector<string> leer_genomas(const string& carpeta_genomas) {
    vector<string> genomas;
    DIR* directorio = opendir(carpeta_genomas.c_str());
    if (directorio == NULL) {
        cerr << "Error: No existe el directorio " << carpeta_genomas << endl;
        return genomas;
    }

    struct dirent* directorio_data;
    while ((directorio_data = readdir(directorio)) != NULL) {
        if (directorio_data->d_name[0] != '.') {
            genomas.push_back(directorio_data->d_name);
        }
    }

    closedir(directorio);
    return genomas;
}

void procesar_genoma_semaforo(const string& genoma_file, float umbral) {
    float promedio = calcular_promedio_GC(genoma_file);
    if (promedio >= umbral) {
        sem_wait(&sem); // Espera a que el semáforo esté disponible
        genomasAceptados.push(genoma_file);
        sem_post(&sem); // Libera el semáforo después de agregar el genoma
    }
}

void consumir_genomas() {
    //cout << "consumidor iniciado" << endl;
    while (!finalizado || !genomasAceptados.empty()) {
        sem_wait(&sem); // Espera a que el semáforo esté disponible
        while (!genomasAceptados.empty()) {
            cout << "Genoma aceptado: " << genomasAceptados.front() << endl;
            genomasAceptados.pop();
            //cout << "while interno" << endl;
        }
        sem_post(&sem); // Libera el semáforo después de consumir genomas
        //cout << "primer while" << endl;
    }
    //cout << "consumidor finalizado" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Error: No se proporcionó un umbral." << endl;
        return 1;
    }

    umbral = atof(argv[1]);
    carpeta_genomas = argv[2];
    if (carpeta_genomas[carpeta_genomas.length() - 1] != '/') carpeta_genomas = carpeta_genomas + '/';
    vector<string> genomas = leer_genomas(carpeta_genomas);
    vector<thread> threads;
    thread consumidor(consumir_genomas);

    sem_init(&sem, 0, 1); // Inicializa el semáforo

    for (const auto& genoma_file : genomas) {
        threads.emplace_back(procesar_genoma_semaforo, genoma_file, umbral);
    }

    for (auto& t : threads) {
        cout << "esperando" << endl;
        t.join();
    }

    sem_wait(&sem); // Espera a que el semáforo esté disponible antes de finalizar
    finalizado = true;
    sem_post(&sem); // Libera el semáforo después de establecer finalizado en true
    consumidor.join();
    sem_destroy(&sem); // Destruye el semáforo

    cout << "consumidor" << endl;
    return 0;
}
