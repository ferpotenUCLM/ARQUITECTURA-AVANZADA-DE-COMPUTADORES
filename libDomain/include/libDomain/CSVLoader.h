#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

#include "Compute.h"

namespace compute {
    struct CellType {
        unsigned int id;
        glm::dvec2 coordinates;
        std::string appearedLocalTime;
    };

    template <typename Cell>
    class CSVLoader {
    public:
        CSVLoader(const std::string& filePath) {
            std::cout << "[...] Abriendo archivo: " << filePath << std::endl;
            
            std::ifstream file(filePath);
            if (!file.is_open()) {
                throw std::runtime_error("No se pudo abrir el archivo: " + filePath);
            }

            std::string line;
            std::getline(file, line); // Saltar header
            
            size_t lineNum = 1;
            while (std::getline(file, line)) {
                lineNum++;
                try {
                    std::stringstream ss(line);
                    std::string token;
                    Cell cell;
                    unsigned int col = 0;
                    
                    while (std::getline(ss, token, ',')) {
                        if (col == 0) {
                            cell.id = std::stoul(token);
                            if (cell.id < 1 || cell.id > 151) {
                                throw std::out_of_range("ID de Pokémon inválido");
                            }
                        } else if (col == 1) {
                            cell.coordinates.x = std::stod(token);
                        } else if (col == 2) {
                            cell.coordinates.y = std::stod(token);
                        } else if (col == 3) {
                            cell.appearedLocalTime = token;
                            break;
                        }
                        col++;
                    }
                    
                    _values.push_back(cell);
                } catch (const std::exception& e) {
                    std::cerr << " Error en línea " << lineNum << ": " << e.what() 
                              << " - Saltando registro" << std::endl;
                    continue;
                }
            }
            
            std::sort(_values.begin(), _values.end(), [](const Cell& a, const Cell& b) {
                return a.appearedLocalTime > b.appearedLocalTime;
            });
            
            std::cout << " Cargados " << _values.size() << " registros" << std::endl;
        } 

        const std::vector<Cell>& data() const { return _values; }

    protected:
        std::vector<Cell> _values;
    };

    using ExerciseLoader = CSVLoader<CellType>;
}
