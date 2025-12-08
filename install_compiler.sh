# Script: install_compiler.sh
#!/bin/bash

echo "=== INSTALANDO COMPILADOR C++ Y DEPENDENCIAS EN EL CLUSTER ==="

NODOS=("AAC15-1" "AAC15-2" "AAC15-3" "AAC15-4")

for nodo in "${NODOS[@]}"; do
    echo "Instalando en $nodo..."
    ssh vmuser@$nodo "sudo apt update && sudo apt install -y build-essential g++ gcc cmake libglm-dev openmpi-bin libopenmpi-dev"
    echo "$nodo completado"
    echo ""
done

echo "Todas las dependencias instaladas en el cluster"
