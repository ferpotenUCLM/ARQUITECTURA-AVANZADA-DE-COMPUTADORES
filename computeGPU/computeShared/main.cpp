#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <omp.h>
#include <libDomain/CSVLoader.h>
#include <libDomain/data/Names.h>
#include <algorithm>
#include <unordered_map>
#include <limits>

using clk = std::chrono::system_clock;

// Función para el ejercicio 1
void exercise1(const compute::ExerciseLoader& csvLoader) {
    std::cout << "\n=== EJERCICIO 1 ===" << std::endl;
    
    std::vector<long long> monoTimes;
    std::vector<long long> parallelTimes;
    
    for (int run = 0; run < 10; run++) {
        // Versión mono-hilo
        auto startMono = clk::now();
        unsigned int pikachuCount = 0;
        for (const auto& cell : csvLoader.data()) {
            if (cell.id == 25) { // Pikachu ID = 25
                pikachuCount++;
            }
        }
        auto endMono = clk::now();
        auto monoTime = std::chrono::duration_cast<std::chrono::milliseconds>(endMono - startMono).count();
        monoTimes.push_back(monoTime);
        
        // Versión paralela con reducción
        auto startParallel = clk::now();
        unsigned int parallelCount = 0;
        const auto& data = csvLoader.data();
        #pragma omp parallel for reduction(+:parallelCount)
        for (size_t i = 0; i < data.size(); i++) {
            if (data[i].id == 25) {
                parallelCount++;
            }
        }
        auto endParallel = clk::now();
        auto parallelTime = std::chrono::duration_cast<std::chrono::milliseconds>(endParallel - startParallel).count();
        parallelTimes.push_back(parallelTime);
        
        if (run == 0) {
            std::cout << "Pikachu count: " << pikachuCount << " (mono), " << parallelCount << " (parallel)" << std::endl;
        }
    }
    
    // Calcular promedios
    long long avgMono = 0, avgParallel = 0;
    for (int i = 0; i < 10; i++) {
        avgMono += monoTimes[i];
        avgParallel += parallelTimes[i];
    }
    avgMono /= 10;
    avgParallel /= 10;
    
    std::cout << "Average times - Mono: " << avgMono << "ms, Parallel: " << avgParallel << "ms" << std::endl;
}

// Función para el ejercicio 2
void exercise2(const compute::ExerciseLoader& csvLoader) {
    std::cout << "\n=== EJERCICIO 2 ===" << std::endl;
    
    std::vector<long long> monoTimes;
    std::vector<long long> parallelTimes;
    
    for (int run = 0; run < 10; run++) {
        // Versión mono-hilo
        auto startMono = clk::now();
        std::vector<unsigned int> countsMono(152, 0);
        for (const auto& cell : csvLoader.data()) {
            if (cell.id >= 1 && cell.id <= 151) {
                countsMono[cell.id]++;
            }
        }
        
        unsigned int maxCount = 0, minCount = std::numeric_limits<unsigned int>::max();
        unsigned int maxId = 0, minId = 0;
        for (unsigned int i = 1; i <= 151; i++) {
            if (countsMono[i] > 0) {
                if (countsMono[i] > maxCount) {
                    maxCount = countsMono[i];
                    maxId = i;
                }
                if (countsMono[i] < minCount) {
                    minCount = countsMono[i];
                    minId = i;
                }
            }
        }
        auto endMono = clk::now();
        auto monoTime = std::chrono::duration_cast<std::chrono::milliseconds>(endMono - startMono).count();
        monoTimes.push_back(monoTime);
        
        // Versión paralela
        auto startParallel = clk::now();
        std::vector<unsigned int> countsParallel(152, 0);
        const auto& data = csvLoader.data();
        #pragma omp parallel
        {
            std::vector<unsigned int> localCounts(152, 0);
            #pragma omp for
            for (size_t i = 0; i < data.size(); i++) {
                unsigned int id = data[i].id;
                if (id >= 1 && id <= 151) {
                    localCounts[id]++;
                }
            }
            
            #pragma omp critical
            {
                for (unsigned int i = 1; i <= 151; i++) {
                    countsParallel[i] += localCounts[i];
                }
            }
        }
        
        unsigned int maxCountPar = 0, minCountPar = std::numeric_limits<unsigned int>::max();
        unsigned int maxIdPar = 0, minIdPar = 0;
        for (unsigned int i = 1; i <= 151; i++) {
            if (countsParallel[i] > 0) {
                if (countsParallel[i] > maxCountPar) {
                    maxCountPar = countsParallel[i];
                    maxIdPar = i;
                }
                if (countsParallel[i] < minCountPar) {
                    minCountPar = countsParallel[i];
                    minIdPar = i;
                }
            }
        }
        auto endParallel = clk::now();
        auto parallelTime = std::chrono::duration_cast<std::chrono::milliseconds>(endParallel - startParallel).count();
        parallelTimes.push_back(parallelTime);
        
        if (run == 0) {
            std::cout << "Most frequent: " << data::stats[maxId] << " (ID: " << maxId << ") with " << maxCount << " appearances" << std::endl;
            std::cout << "Least frequent: " << data::stats[minId] << " (ID: " << minId << ") with " << minCount << " appearances" << std::endl;
        }
    }
    
    // Calcular promedios
    long long avgMono = 0, avgParallel = 0;
    for (int i = 0; i < 10; i++) {
        avgMono += monoTimes[i];
        avgParallel += parallelTimes[i];
    }
    avgMono /= 10;
    avgParallel /= 10;
    
    std::cout << "Average times - Mono: " << avgMono << "ms, Parallel: " << avgParallel << "ms" << std::endl;
}

// Función para el ejercicio 3
void exercise3(const compute::ExerciseLoader& csvLoader, unsigned int numThreads) {
    std::cout << "\n=== EJERCICIO 3 ===" << std::endl;
    
    omp_set_num_threads(numThreads);
    
    const glm::dvec2 targetCoord(20.525750, -97.46000);
    const double radius = 30.0;
    
    // Obtener las 1000 apariciones más recientes (primeras 1000 del dataset ordenado)
    std::vector<compute::CellType> recentAppearances;
    size_t count = std::min(size_t(1000), csvLoader.data().size());
    for (size_t i = 0; i < count; i++) {
        recentAppearances.push_back(csvLoader.data()[i]);
    }
    
    // Encontrar el más cercano en las apariciones recientes
    double minDistance = std::numeric_limits<double>::max();
    compute::CellType closestPokemon;
    bool found = false;
    
    const auto& recentData = recentAppearances;
    #pragma omp parallel for
    for (size_t i = 0; i < recentData.size(); i++) {
        double distance = compute::calculateDistance(recentData[i].coordinates, targetCoord);
        if (distance <= radius) {
            #pragma omp critical
            {
                if (distance < minDistance) {
                    minDistance = distance;
                    closestPokemon = recentData[i];
                    found = true;
                }
            }
        }
    }
    
    if (found) {
        std::cout << "Closest Pokemon in recent appearances: " << data::stats[closestPokemon.id] 
                  << " (ID: " << closestPokemon.id << ") at distance: " << minDistance << " km" << std::endl;
    } else {
        std::cout << "No Pokemon found within 30km radius in recent appearances" << std::endl;
    }
}

// Función para el ejercicio 4a (tiles)
void exercise4a(const compute::ExerciseLoader& csvLoader, unsigned int numThreads) {
    std::cout << "\n=== EJERCICIO 4a ===" << std::endl;
    
    unsigned int tileThreads = numThreads / 4;
    if (tileThreads < 1) tileThreads = 1;
    
    const glm::dvec2 targetCoord(20.525750, -97.46000);
    const double radius = 30.0;
    
    // Obtener las 1000 apariciones más recientes
    std::vector<compute::CellType> recentAppearances;
    size_t count = std::min(size_t(1000), csvLoader.data().size());
    for (size_t i = 0; i < count; i++) {
        recentAppearances.push_back(csvLoader.data()[i]);
    }
    
    double minDistance = std::numeric_limits<double>::max();
    compute::CellType closestPokemon;
    bool found = false;
    
    const auto& recentData = recentAppearances;
    #pragma omp parallel num_threads(4)
    {
        int tile = omp_get_thread_num();
        unsigned int tileSize = recentData.size() / 4;
        unsigned int tileStart = tile * tileSize;
        unsigned int tileEnd = (tile == 3) ? recentData.size() : (tile + 1) * tileSize;
        
        #pragma omp parallel for num_threads(tileThreads)
        for (size_t i = tileStart; i < tileEnd; i++) {
            double distance = compute::calculateDistance(recentData[i].coordinates, targetCoord);
            if (distance <= radius) {
                #pragma omp critical
                {
                    if (distance < minDistance) {
                        minDistance = distance;
                        closestPokemon = recentData[i];
                        found = true;
                    }
                }
            }
        }
    }
    
    if (found) {
        std::cout << "Closest Pokemon (with tiles): " << data::stats[closestPokemon.id] 
                  << " (ID: " << closestPokemon.id << ") at distance: " << minDistance << " km" << std::endl;
    } else {
        std::cout << "No Pokemon found within 30km radius (with tiles)" << std::endl;
    }
}

// Función para el ejercicio 4b (loop unrolling)
void exercise4b(const compute::ExerciseLoader& csvLoader, unsigned int numThreads) {
    std::cout << "\n=== EJERCICIO 4b ===" << std::endl;
    
    omp_set_num_threads(numThreads);
    
    const glm::dvec2 targetCoord(20.525750, -97.46000);
    const double radius = 30.0;
    
    // Obtener las 1000 apariciones más recientes
    std::vector<compute::CellType> recentAppearances;
    size_t count = std::min(size_t(1000), csvLoader.data().size());
    for (size_t i = 0; i < count; i++) {
        recentAppearances.push_back(csvLoader.data()[i]);
    }
    
    double minDistance = std::numeric_limits<double>::max();
    compute::CellType closestPokemon;
    bool found = false;
    
    const auto& recentData = recentAppearances;
    #pragma omp parallel for
    for (size_t i = 0; i < recentData.size(); i += 4) {
        // Loop unrolling - procesar 4 elementos por iteración
        for (int j = 0; j < 4 && (i + j) < recentData.size(); j++) {
            double distance = compute::calculateDistance(recentData[i + j].coordinates, targetCoord);
            if (distance <= radius) {
                #pragma omp critical
                {
                    if (distance < minDistance) {
                        minDistance = distance;
                        closestPokemon = recentData[i + j];
                        found = true;
                    }
                }
            }
        }
    }
    
    if (found) {
        std::cout << "Closest Pokemon (with loop unrolling): " << data::stats[closestPokemon.id] 
                  << " (ID: " << closestPokemon.id << ") at distance: " << minDistance << " km" << std::endl;
    } else {
        std::cout << "No Pokemon found within 30km radius (with loop unrolling)" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if ( argc < 2 )
    {
        std::cout << "CSV file not found. Please use the first argument to set the file to be processed..." << std::endl;
        return -1;
    }

    std::string filePath = argv[1];
    compute::ExerciseLoader csvLoader( filePath );

    const unsigned int maxThreads = omp_get_num_procs();
    printf("Available threads: %i\n", maxThreads);

    unsigned int numThreads { maxThreads / 2 };

    if( argc == 3)
    {
        numThreads = std::stoi( argv[2]);
    }

    // Ejercicio 1
    exercise1(csvLoader);
    
    // Ejercicio 2
    exercise2(csvLoader);
    
    // Ejercicio 3
    exercise3(csvLoader, numThreads);
    
    // Ejercicio 4a
    exercise4a(csvLoader, numThreads);
    
    // Ejercicio 4b
    exercise4b(csvLoader, numThreads);

    return 0;
}