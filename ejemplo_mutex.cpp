#include <iostream>
#include <pthread.h> 
#include <unistd.h> //sleep
#include <string.h> //strerror
#include <vector>
#include <math.h>
#include "metrictime.hpp"
using namespace std; 

const int numThreads = 4; 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger la variable suma

typedef struct{//definimos una estructura para guardar los limites de las hebras 
    vector<unsigned int>* v; 
    unsigned int l; 
    unsigned int r;
    unsigned int id; 
    uint64_t* sum; 
}Data;

void *fun(void *arg) {
    Data* datos = (Data*) arg;
    
    for(size_t i = datos->l; i < datos->r; i++) {
        pthread_mutex_lock(&mutex); // Bloquea el mutex para garantizar exclusión mutua
        *(datos->sum) += datos->v->at(i);
        pthread_mutex_unlock(&mutex); // Libera el mutex
    }
    return NULL;
}

int main() {
    pthread_t tId[numThreads];
    Data d[numThreads];
    uint64_t suma = 0; 
    vector<unsigned int> nums; 

    for(size_t i = 1; i <= pow(10,8); i++) nums.push_back(i);  // Inicializa el vector

    TIMERSTART(PARALELO);
    for(int i = 0; i < numThreads; i++){
        d[i].id = i;
        d[i].v = &nums; 
        d[i].l = nums.size() / numThreads*i; 
        if(i == numThreads-1) d[i].r = nums.size();
        else d[i].r = nums.size() / numThreads*(i + 1); 
        d[i].sum = &suma; 

        int ret = pthread_create(&tId[i], NULL, &fun, &d[i]); // Crea hebras con Id tId[i], configuracion por defecto, las hebras ejecutaran la funcion fun con argumentos &d[i]
        if (ret) {
            cout <<"error al crear hilo: " << strerror(ret);
            return ret;
        }
    }
    uint64_t sum = 0;
    for(int i = 0; i < numThreads; i++){
        int ret = pthread_join(tId[i], NULL); // Espera a que las hebras terminen
        
        if (ret) {
            cout << "hilo no pudo hacer join : " << strerror(ret) << endl;
            return ret;
        }
    }
    cout << "suma total:" << suma << endl;
    TIMERSTOP(PARALELO);

    TIMERSTART(Secuencial);
    sum = 0; 
    for(auto i : nums) sum += i; 

    cout << "Suma: " << sum << endl; 
    TIMERSTOP(Secuencial);
    return 0;
}