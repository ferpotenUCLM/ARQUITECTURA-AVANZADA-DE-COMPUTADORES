#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "Compute.h"

namespace compute
{
    struct CellType
    {
        unsigned int id;
        glm::dvec2 coordinates; // Latitude, longitude
        std::string appearedLocalTime; // Para ordenar por tiempo
    };

    template <typename Cell>
    class CSVLoader
    {
    public:
        CSVLoader( const std::string& filePath )
        {
            std::cout << "Abriendo Archivo ..." << filePath << std::endl;

            std::ifstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Error abriendo archivo!" << std::endl;
                return;
            }

            std::string line;
            // Skip header
            std::getline(file, line);

            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string token;
                
                Cell cell;
                unsigned int col = 0;
                
                while (std::getline(ss, token, ',')) {
                    if (col == 0) { // pokemonId
                        cell.id = std::stoi(token);
                    } else if (col == 1) { // latitude
                        cell.coordinates.x = std::stod(token);
                    } else if (col == 2) { // longitude
                        cell.coordinates.y = std::stod(token);
                    } else if (col == 3) { // appearedLocalTime
                        cell.appearedLocalTime = token;
                        break; // Solo necesitamos las primeras 4 columnas
                    }
                    col++;
                }
                
                _values.push_back(cell);
            }
            
            // Ordenar por tiempo (más recientes primero)
            std::sort(_values.begin(), _values.end(), [](const Cell& a, const Cell& b) {
                return a.appearedLocalTime > b.appearedLocalTime;
            });
            
            std::cout << "Se han cargado " << _values.size() << " records" << std::endl;
        } 

        const std::vector<Cell>& data( void ) const { return _values; }

    protected:
        std::vector<Cell> _values;
    };

    using ExerciseLoader = CSVLoader<CellType>;
    using ExerciseResults = std::vector<CellType>;
}