#!/bin/bash

NUM_PROCESOS=${1:-6}
ARCHIVO_CSV=${2:-"data/dataset.csv"}

# Crear hostfile dinámico para especificar slots
HOSTFILE=$(mktemp)
echo "AAC15-1 slots=4" > $HOSTFILE
echo "AAC15-2 slots=4" >> $HOSTFILE
echo "AAC15-3 slots=4" >> $HOSTFILE
echo "AAC15-4 slots=4" >> $HOSTFILE

echo "=== EJECUTANDO PRÁCTICA 1b ==="
echo "Procesos MPI: $NUM_PROCESOS"
echo "Archivo: $ARCHIVO_CSV"
echo "Hostfile generado:"
cat $HOSTFILE
echo ""; echo "Iniciando ejecución..."
echo "────────────────────────────────────"

# Verificar ejecutable
if [ ! -f "./computeDistributed/computeDistributed" ]; then
    echo "Error: Ejecutable no encontrado. Ejecuta ./scripts/build_cluster.sh"
    rm -f $HOSTFILE
    exit 1
fi

# Ejecutar con hostfile
mpirun -np $NUM_PROCESOS --hostfile $HOSTFILE ./computeDistributed/computeDistributed $ARCHIVO_CSV

rm -f $HOSTFILE
echo ""; echo "===EJECUCIÓN FINALIZADA ==="
