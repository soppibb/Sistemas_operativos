#include <iostream>
#include <thread> 
#include <unistd.h> //sleep
#include <string.h> //strerror
#include <vector>
#include <math.h>
#include <mutex>
#include "metrictime.hpp" // Importa las funciones de temporización

using namespace std; 

const int numThreads = 4; 
mutex m; // Mutex para proteger la variable 'suma'

typedef struct{
    vector<unsigned int>* v; 
    unsigned int l; 
    unsigned int r;
    unsigned int id; 
    uint64_t* sum; 
}Data;

// Función que realiza la suma en una sección del vector
void fun(vector<unsigned int>* v, size_t l, size_t r, uint64_t* suma) { 
    for(size_t i = l; i < r; i++) {
        m.lock(); // Bloquea el mutex para garantizar exclusión mutua
        *(suma) += v->at(i);
        m.unlock(); // Libera el mutex
    }
    return;
}

int main() {
    vector<thread> _t; // Vector de hilos
    uint64_t suma = 0; 

    vector<unsigned int> nums; 
    for(size_t i = 1; i <= pow(10,8); i++) nums.push_back(i);  // Inicializa el vector

    TIMERSTART(PARALELO); // Inicia el temporizador para medir el tiempo paralelo
    for(int i = 0; i < numThreads; i++){
        size_t l = nums.size() / numThreads*i; 
        size_t r = nums.size() / numThreads*(i + 1);
        if(i == numThreads-1) r = nums.size();
        _t.push_back(thread(fun, &nums, l, r, &suma )); // Crea hilos y los agrega al vector
    }
  
    for(auto& i : _t) i.join(); // Espera a que todos los hilos terminen
    cout << "suma total:" << suma << endl; // Imprime la suma total calculada en paralelo
    TIMERSTOP(PARALELO); // Detiene el temporizador para el tiempo paralelo

    uint64_t sum = 0; // Variable para la suma secuencial
    TIMERSTART(Secuencial); // Inicia el temporizador para medir el tiempo secuencial
    sum = 0; 
    for(auto i : nums) sum += i; // Calcula la suma secuencial
    cout << "Suma: " << sum << endl; // Imprime la suma secuencial
    TIMERSTOP(Secuencial); // Detiene el temporizador para el tiempo secuencial
    return 0;
}
