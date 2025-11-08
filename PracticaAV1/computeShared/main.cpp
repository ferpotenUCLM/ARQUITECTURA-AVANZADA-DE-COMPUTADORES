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
void ejercicio1(const compute::ExerciseLoader& csvLoader) {
    printf("\n=== EJERCICIO 1 ===\n");
    // Versión mono-hilo
    auto startMono = clk::now();
    unsigned int conteopikachu = 0;
    for (const auto& cell : csvLoader.data()) {
        if (cell.id == 25) { // Pikachu ID = 25
            conteopikachu++;
        }
    }
    auto endMono = clk::now();
    std::chrono::duration<double, std::milli> tiempoMONO = endMono - startMono;
    
       
    // Versión paralela con reducción
    auto startParallel = clk::now();
    unsigned int conteopikachuparalelo = 0;
    const auto& data = csvLoader.data();
    #pragma omp parallel for reduction(+:conteopikachuparalelo) schedule(guided,9000) //varios hilos y evitamos carrera ç
    //schedule(guided,10000) schedule(guided,9000) schedule(dynamic,500) los mejores si mas o menos ya sale peor
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i].id == 25) {
            conteopikachuparalelo++;
        }
    }
    auto endParallel = clk::now();
    std::chrono::duration<double, std::milli> tiempoparalelo = endParallel - startParallel;
    printf("Conteo de Pikachus: %u (mono), %u (paralelo)\n", conteopikachu, conteopikachuparalelo);
    printf("Tiempos de ejecucion mono: %.4f ms, Paralelo: %.4f ms\n",
    tiempoMONO.count(), tiempoparalelo.count());
}

// Función para el ejercicio 2
void ejercicio2(const compute::ExerciseLoader& csvLoader) {
    printf("\n=== EJERCICIO 2 ===\n");
    



    // --- Versión mono-hilo ---
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
        std::chrono::duration<double, std::milli> tiempoMONO = endMono - startMono;
      

        //Version Paralela
        auto startParallel = clk::now();

        std::vector<unsigned int> countsParallel(152, 0);
        const auto& data = csvLoader.data();

        #pragma omp parallel
        {
            std::vector<unsigned int> localCounts(152, 0);

            #pragma omp for schedule(dynamic, 1000)
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
        std::chrono::duration<double, std::milli> parallelTime = endParallel - startParallel;


        // Solo mostrar resultados descriptivos en la primera ejecución
     
            printf("Más frecuente: %s (ID: %u) con %u apariciones\n",
                   data::stats[maxId].c_str(), maxId, maxCount);
            printf("Menos frecuente: %s (ID: %u) con  %u apariciones\n",
                   data::stats[minId].c_str(), minId, minCount);

        printf("Mono: %.3f ms, Paralelo: %.3f ms\n", tiempoMONO.count(), parallelTime.count());
}

// Función para el ejercicio 3
void ejercicio3(const compute::ExerciseLoader& csvLoader, unsigned int numThreads) {
    printf("\n=== EJERCICIO 3 ===\n");
    //seteamos el numero de hilos
    omp_set_num_threads(numThreads);
    //carga datos espaciales
    const glm::dvec2 targetCoordenad(20.525750, -97.46000);
    const double radio = 30.0;

    // Medir tiempo total
    auto inicio = clk::now();

    // Obtener las 1000 apariciones más recientes (primeras 1000 del dataset ordenado)
    std::vector<compute::CellType> aparicionesRecientes;
    size_t count = std::min(size_t(1000), csvLoader.data().size()); 
    aparicionesRecientes.reserve(count); //guardamos en apariciones recientes

    for (size_t i = 0; i < count; i++) {
        aparicionesRecientes.push_back(csvLoader.data()[i]);
    }

    // Encontrar el más cercano dentro del radio
    double minDistance = std::numeric_limits<double>::max(); // valor +infinito para el contador
    compute::CellType closestPokemon;
    bool enocntrado = false; // flag encontrado se inicia a false

    const auto& recentData = aparicionesRecientes;

    #pragma omp parallel for schedule(dynamic, 100)
    for (size_t i = 0; i < recentData.size(); i++) {
        double distance = compute::calculateDistance(recentData[i].coordinates, targetCoordenad); 
        //calculamos las distancia entre las coordenadas y el target
        if (distance <= radio) {
            #pragma omp critical
            {
                if (distance < minDistance) {
                    minDistance = distance;
                    closestPokemon = recentData[i];
                    enocntrado = true;
                }
            }
        }
    }

    auto end = clk::now();
    std::chrono::duration<double, std::milli> execTime = end - inicio;

    if (enocntrado) {
        printf("Pokemon más cercano en apariciones recientes : %s (ID: %u) a una distancia de : %.3f km\n",
               data::stats[closestPokemon.id].c_str(),
               closestPokemon.id,
               minDistance);
    } else {
        printf("No se han encontrado Pokémon a menos de %.1f km de radio in apariciones recientes\n", radio);
    }

    printf("Tiempo de ejecucion: %.3f ms\n", execTime.count());
}

//
void ejercicio4a(const compute::ExerciseLoader& csvLoader, unsigned int numThreads) {
    printf("\n=== EJERCICIO 4a ===\n");

    const glm::dvec2 targetCoord(20.525750, -97.46000);
    const double radio = 30.0;

    // Dividir threads en 4 tiles
    unsigned int tileThreads =  numThreads / 4; 
    unsigned int totalTiles = 4;

    // Obtener las 1000 apariciones más recientes
    std::vector<compute::CellType> recentAppearances;
    size_t count = std::min(size_t(1000), csvLoader.data().size());
    recentAppearances.reserve(count);

    for (size_t i = 0; i < count; i++) {
        recentAppearances.push_back(csvLoader.data()[i]);
    }

    const auto& recentData = recentAppearances;
    double minDistance = std::numeric_limits<double>::max();
    compute::CellType closestPokemon;
    bool found = false;

    auto start = clk::now();

    // Procesamiento dividido en tiles
    #pragma omp parallel for num_threads(totalTiles) schedule(static)
    for (int tile = 0; tile < totalTiles; tile++) {// iteracion para division de las tiles
        unsigned int tileTam = recentData.size() / totalTiles;  //
        unsigned int tileIncio = tile * tileTam;
        unsigned int tileFIN;
        if (tile == totalTiles - 1) {
        // Último tile: llega hasta el final del dataset
        tileFIN = recentData.size();
        } else {
        // Tiles intermedios: llegan hasta el inicio del siguiente bloque
        tileFIN = (tile + 1) * tileTam;
        }
        //Paralelismo dentro del tile
        #pragma omp parallel for num_threads(tileThreads) schedule(dynamic, 50)
        for (size_t i = tileIncio; i < tileFIN; i++) {
            double distance = compute::calculateDistance(recentData[i].coordinates, targetCoord);
            if (distance <= radio) {
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

    auto end = clk::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    if (found) {
        printf("Pokemon más cercano en apariciones recientes: %s (ID: %u) a una distancia de %.3f km\n", 
               data::stats[closestPokemon.id].c_str(), closestPokemon.id, minDistance);
    } else {
        printf("No se han encontrado Pokémon a menos de %.1f km de radio in apariciones recientes\n", radio);
    }

    printf("iempo de ejecucion: %.3f ms\n", elapsed.count());
}

// === EJERCICIO 4b: Loop Unrolling ===
void ejercicio4b(const compute::ExerciseLoader& csvLoader, unsigned int numThreads) {
    printf("\n=== EJERCICIO 4b (Loop Unrolling) ===\n");

    omp_set_num_threads(numThreads);

    const glm::dvec2 targetCoord(20.525750, -97.46000);
    const double radio = 30.0;

    // Obtener las 1000 apariciones más recientes
    std::vector<compute::CellType> recentAppearances;
    size_t count = std::min(size_t(1000), csvLoader.data().size());
    recentAppearances.reserve(count);
    for (size_t i = 0; i < count; i++) {
        recentAppearances.push_back(csvLoader.data()[i]);
    }

    const auto& recentData = recentAppearances;
    double minDistance = std::numeric_limits<double>::max();
    compute::CellType closestPokemon;
    bool found = false;

    auto start = clk::now();

    #pragma omp parallel for schedule(dynamic, 50)
    for (size_t i = 0; i < recentData.size(); i += 4) {
        double localMin = std::numeric_limits<double>::max();
        compute::CellType localClosest;
        bool localFound = false;

        // Loop unrolling manual: procesar 4 elementos por iteración
        for (int j = 0; j < 4 && (i + j) < recentData.size(); j++) {
            double distance = compute::calculateDistance(recentData[i + j].coordinates, targetCoord);
            if (distance <= radio && distance < localMin) {
                localMin = distance;
                localClosest = recentData[i + j];
                localFound = true;
            }
        }

        // Combinar resultados locales
        if (localFound) {
            #pragma omp critical
            {
                if (localMin < minDistance) {
                    minDistance = localMin;
                    closestPokemon = localClosest;
                    found = true;
                }
            }
        }
    }

    auto end = clk::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    if (found) {
        printf("Pokemon más cercano en apariciones recientes: %s (ID: %u) a una distancia de %.3f km\n",
               data::stats[closestPokemon.id].c_str(), closestPokemon.id, minDistance);
    } else {
           printf("No se han encontrado Pokémon a menos de %.1f km de radio in apariciones recientes\n", radio);
    }

    printf("Tiempo de ejecucion desenrollado de bucles: %.3f ms\n", elapsed.count());
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
    ejercicio1(csvLoader);
    ejercicio2(csvLoader);
    ejercicio3(csvLoader, numThreads);
    ejercicio4a(csvLoader, numThreads);
    ejercicio4b(csvLoader, numThreads);
    return 0;
}