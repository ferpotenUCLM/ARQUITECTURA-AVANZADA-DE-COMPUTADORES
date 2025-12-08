#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <mpi.h>
#include <libDomain/CSVLoader.h>
#include <libDomain/data/Names.h>
#include <algorithm>
#include <unordered_map>
#include <limits>
#include <sstream>
#include <iomanip>
#include <fstream>

using clk = std::chrono::system_clock;

// Estructura extendida para MPI
struct DistributedPokemon {
    unsigned int id;
    glm::dvec2 coordinates;
    std::string appearedLocalTime;
    std::string continent;
    
    std::string serialize() const {
        std::stringstream ss;
        ss << id << "," << coordinates.x << "," << coordinates.y << "," 
           << appearedLocalTime << "," << continent;
        return ss.str();
    }
    
    static DistributedPokemon deserialize(const std::string& data) {
        std::stringstream ss(data);
        std::string token;
        DistributedPokemon p;
        std::getline(ss, token, ','); p.id = std::stoul(token);
        std::getline(ss, token, ','); p.coordinates.x = std::stod(token);
        std::getline(ss, token, ','); p.coordinates.y = std::stod(token);
        std::getline(ss, token, ','); p.appearedLocalTime = token;
        std::getline(ss, token, ','); p.continent = token;
        return p;
    }
};

// Clasificación geográfica.
std::string determineContinent(const glm::dvec2& coords) {
    double lat = coords.x, lon = coords.y;
    
    if ((lat >= 15.0 && lat <= 75.0 && lon >= -170.0 && lon <= -50.0) || // N. América
        (lat >= -56.0 && lat < 15.0 && lon >= -85.0 && lon <= -35.0)) {   // S. América
        return "America";
    }
    if (lat >= 36.0 && lat <= 72.0 && lon >= -10.0 && lon <= 60.0) {
        return "Europe";
    }
    if (lat >= -35.0 && lat <= 37.0 && lon >= -17.0 && lon <= 52.0) {
        return "Africa";
    }
    if (lat >= 10.0 && lat <= 75.0 && lon > 60.0 && lon <= 180.0) {
        return "Asia";
    }
    if (lat >= -50.0 && lat <= -10.0 && lon >= 110.0 && lon <= 180.0) {
        return "Pacific";
    }
    return "Other";
}

// Validación de archivo
bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

// Serialización batch eficiente
std::vector<char> serializeAllData(const std::vector<DistributedPokemon>& data) {
    std::string allStr;
    allStr.reserve(data.size() * 100); // Pre-allocar
    for (const auto& p : data) {
        allStr += p.serialize() + "\n";
    }
    return std::vector<char>(allStr.begin(), allStr.end());
}

// Calcular distribución balanceada
void calculateDistribution(int totalSize, int numProcs, 
                          std::vector<int>& sendCounts, 
                          std::vector<int>& displacements) {
    sendCounts.resize(numProcs);
    displacements.resize(numProcs);
    int base = totalSize / numProcs;
    int remainder = totalSize % numProcs;
    
    int offset = 0;
    for (int i = 0; i < numProcs; i++) {
        sendCounts[i] = base + (i < remainder ? 1 : 0);
        displacements[i] = offset;
        offset += sendCounts[i];
    }
}

// Distribuir y contar Pokémon (Ejercicio 2a/b)
void distributedPokemonCount(int rank, int size, const std::vector<DistributedPokemon>& allData) {
    MPI_Barrier(MPI_COMM_WORLD);
    auto start = clk::now();
    
    std::vector<DistributedPokemon> localData;
    
    if (rank == 0) {
        // Distribuir usando lógica manual (compatible con cualquier MPI)
        int recordsPerProc = allData.size() / size;
        int remainder = allData.size() % size;
        int startIdx = 0;
        
        for (int dest = 0; dest < size; dest++) {
            int localCount = recordsPerProc + (dest < remainder ? 1 : 0);
            
            if (dest == 0) {
                // Root asigna a sí mismo
                localData.assign(allData.begin() + startIdx, 
                               allData.begin() + startIdx + localCount);
            } else {
                // Enviar a otros procesos
                MPI_Send(&localCount, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
                for (int i = 0; i < localCount; i++) {
                    std::string serialized = allData[startIdx + i].serialize();
                    int len = serialized.size();
                    MPI_Send(&len, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
                    MPI_Send(serialized.c_str(), len, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
                }
            }
            startIdx += localCount;
        }
    } else {
        // Recibir datos
        int localCount;
        MPI_Recv(&localCount, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        localData.reserve(localCount);
        
        for (int i = 0; i < localCount; i++) {
            int len;
            MPI_Recv(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::vector<char> buffer(len + 1);
            MPI_Recv(buffer.data(), len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buffer[len] = '\0';
            localData.push_back(DistributedPokemon::deserialize(std::string(buffer.data())));
        }
    }
    
    // Conteo local
    std::unordered_map<unsigned int, unsigned int> localCounts;
    for (const auto& p : localData) localCounts[p.id]++;
    
    // Recolectar resultados
    if (rank == 0) {
        auto globalCounts = localCounts;
        
        for (int src = 1; src < size; src++) {
            int numEntries;
            MPI_Recv(&numEntries, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < numEntries; i++) {
                unsigned int id, count;
                MPI_Recv(&id, 1, MPI_UNSIGNED, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&count, 1, MPI_UNSIGNED, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                globalCounts[id] += count;
            }
        }
        
        auto end = clk::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        unsigned int total = 0;
        for (const auto& [id, count] : globalCounts) total += count;
        
        if (rank == 0) {
            std::cout << "\n=== EJERCICIO 2 - RECUENTO DISTRIBUIDO ===" << std::endl;
            std::cout << "Total contado: " << total << " / Original: " << allData.size() << std::endl;
            std::cout << "Tiempo: " << duration << " ms" << std::endl;
        }
    } else {
        int numEntries = localCounts.size();
        MPI_Send(&numEntries, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (const auto& [id, count] : localCounts) {
            MPI_Send(&id, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&count, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
        }
    }
}

// Pokémon más raro por continente (Ejercicio 3)
void rarestPokemonByContinent(int rank, int size, const std::vector<DistributedPokemon>& allData) {
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        std::cout << "\n=== EJERCICIO 3 - POKÉMON MÁS RARO POR CONTINENTE ===" << std::endl;
        
        std::unordered_map<std::string, std::vector<DistributedPokemon>> continentData;
        for (const auto& p : allData) {
            if (p.continent != "Other") continentData[p.continent].push_back(p);
        }
        
        const std::vector<std::string> continents = {"America", "Europe", "Africa", "Asia", "Pacific"};
        std::vector<DistributedPokemon> rarestPokemons;
        std::vector<std::string> continentNames;
        
        // Asignar continentes a procesos
        for (size_t i = 0; i < continents.size() && i < size - 1; i++) {
            int dest = i + 1;
            const auto& continent = continents[i];
            const auto& data = continentData[continent];
            
            int contLen = continent.size();
            MPI_Send(&contLen, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(continent.c_str(), contLen, MPI_CHAR, dest, 1, MPI_COMM_WORLD);
            
            int dataSize = data.size();
            MPI_Send(&dataSize, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            
            for (const auto& p : data) {
                std::string s = p.serialize();
                int len = s.size();
                MPI_Send(&len, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
                MPI_Send(s.c_str(), len, MPI_CHAR, dest, 1, MPI_COMM_WORLD);
            }
        }
        
        // Recibir resultados
        for (size_t i = 0; i < continents.size() && i < size - 1; i++) {
            int src = i + 1;
            int found;
            MPI_Recv(&found, 1, MPI_INT, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            if (found) {
                int contLen, pokeLen;
                MPI_Recv(&contLen, 1, MPI_INT, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                std::vector<char> contBuf(contLen + 1);
                MPI_Recv(contBuf.data(), contLen, MPI_CHAR, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                contBuf[contLen] = '\0';
                
                MPI_Recv(&pokeLen, 1, MPI_INT, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                std::vector<char> pokeBuf(pokeLen + 1);
                MPI_Recv(pokeBuf.data(), pokeLen, MPI_CHAR, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                pokeBuf[pokeLen] = '\0';
                
                rarestPokemons.push_back(DistributedPokemon::deserialize(std::string(pokeBuf.data())));
                continentNames.push_back(std::string(contBuf.data()));
            }
        }
        
        // Calcular ruta (algoritmo voráz)
        if (rarestPokemons.size() > 1) {
            std::cout << "\nRuta óptima (vecino más cercano):" << std::endl;
            double totalDist = 0;
            std::vector<bool> visited(rarestPokemons.size(), false);
            int current = 0;
            visited[0] = true;
            
            std::cout << "1. " << data::stats[rarestPokemons[0].id] 
                      << " (" << continentNames[0] << ")\n";
            
            for (size_t step = 1; step < rarestPokemons.size(); step++) {
                int next = -1;
                double minDist = std::numeric_limits<double>::max();
                
                for (size_t i = 0; i < rarestPokemons.size(); i++) {
                    if (!visited[i]) {
                        double d = compute::calculateDistance(
                            rarestPokemons[current].coordinates, 
                            rarestPokemons[i].coordinates);
                        if (d < minDist) {
                            minDist = d;
                            next = i;
                        }
                    }
                }
                
                if (next != -1) {
                    visited[next] = true;
                    totalDist += minDist;
                    std::cout << step + 1 << ". " << data::stats[rarestPokemons[next].id] 
                              << " (" << continentNames[next] << ") - " 
                              << std::fixed << std::setprecision(2) << minDist << " km\n";
                    current = next;
                }
            }
            std::cout << "Distancia total: " << std::fixed << std::setprecision(2) 
                      << totalDist << " km" << std::endl;
        }
    }
    else if (rank < 6) { // Procesos 1-5
        int contLen;
        MPI_Recv(&contLen, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::vector<char> contBuf(contLen + 1);
        MPI_Recv(contBuf.data(), contLen, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        contBuf[contLen] = '\0';
        std::string continent(contBuf.data());
        
        int dataSize;
        MPI_Recv(&dataSize, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        std::vector<DistributedPokemon> localData;
        localData.reserve(dataSize);
        
        for (int i = 0; i < dataSize; i++) {
            int len;
            MPI_Recv(&len, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::vector<char> buf(len + 1);
            MPI_Recv(buf.data(), len, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buf[len] = '\0';
            localData.push_back(DistributedPokemon::deserialize(std::string(buf.data())));
        }
        
        // Encontrar más raro
        std::unordered_map<unsigned int, unsigned int> freq;
        for (const auto& p : localData) freq[p.id]++;
        
        unsigned int minFreq = std::numeric_limits<unsigned int>::max();
        DistributedPokemon rarest;
        bool found = false;
        
        for (const auto& p : localData) {
            if (freq[p.id] < minFreq) {
                minFreq = freq[p.id];
                rarest = p;
                found = true;
            }
        }
        
        MPI_Send(&found, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        if (found) {
            int len = continent.size();
            MPI_Send(&len, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(continent.c_str(), len, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
            
            std::string s = rarest.serialize();
            len = s.size();
            MPI_Send(&len, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(s.c_str(), len, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Validación robusta de argumentos
    if (argc < 2) {
        if (rank == 0) {
            std::cerr << "ERROR: Archivo CSV no especificado\n";
            std::cerr << "Uso: " << argv[0] << " <ruta/al/dataset.csv>\n";
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
    // Broadcast del path del archivo
    char csvPath[256];
    strncpy(csvPath, argv[1], 255);
    csvPath[255] = '\0';
    MPI_Bcast(csvPath, 256, MPI_CHAR, 0, MPI_COMM_WORLD);
    
    // Verificación de archivo (solo root)
    if (rank == 0 && !fileExists(csvPath)) {
        std::cerr << "ERROR: Archivo no encontrado: " << csvPath << std::endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    // Cargar datos solo en root
    std::vector<DistributedPokemon> allData;
    if (rank == 0) {
        compute::ExerciseLoader csvLoader(csvPath);
        const auto& loaded = csvLoader.data();
        allData.reserve(loaded.size());
        
        for (const auto& cell : loaded) {
            DistributedPokemon dp;
            dp.id = cell.id;
            dp.coordinates = cell.coordinates;
            dp.appearedLocalTime = cell.appearedLocalTime;
            dp.continent = determineContinent(cell.coordinates);
            allData.push_back(dp);
        }
        
        std::cout << "Procesando " << allData.size() << " registros con " 
                  << size << " procesos MPI" << std::endl;
    }
    
    // Ejecutar ejercicios
    distributedPokemonCount(rank, size, allData);
    rarestPokemonByContinent(rank, size, allData);
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}
