#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <dirent.h>

std::mutex mtx; // mutex para asegurar la exclusión mutua
std::vector<std::string> directories = {"dir1", "dir2", "dir3", "dir4"};
int current_directory = 0; // índice del directorio actual

void processDirectory(const std::string& directory) {
    // Simula el procesamiento del directorio
    // Puedes realizar cualquier tarea específica aquí
    std::cout << "Procesando directorio: " << directory << std::endl;
}

void threadFunction(int threadId) {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        if (current_directory >= directories.size()) {
            // Todos los directorios han sido procesados
            break;
        }

        std::string currentDir = directories[current_directory];
        current_directory++;

        lock.unlock();

        // Realizar el procesamiento del directorio
        processDirectory(currentDir);
    }
}

int main() {
    const int numThreads = 4;
    std::vector<std::thread> threads;

    // Crear hebras
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(threadFunction, i);
    }

    // Unirse a las hebras
    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}