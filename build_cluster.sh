#!/bin/bash

# Script de compilación para el cluster
echo "=== COMPILANDO PRÁCTICA 1b EN EL CLUSTER ==="

# Crear directorio de build
mkdir -p build
cd build

# Configurar con CMake
echo "Configurando proyecto con CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compilar
echo "Compilando proyecto..."
make -j4

echo "Compilación completada!"
echo "Ejecutables generados:"
echo "  - computeShared (Práctica 1a)"