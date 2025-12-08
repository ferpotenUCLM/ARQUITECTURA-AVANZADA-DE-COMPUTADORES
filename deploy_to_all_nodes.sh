#!/bin/bash

# Script maestro para desplegar y ejecutar en todos los nodos del cluster
# Uso: ./deploy_to_all_nodes.sh [num_procesos] [archivo_csv]

NUM_PROCESOS=${1:-16}  # Por defecto 16 procesos (4 nodos x 4 cores)
ARCHIVO_CSV=${2:-"data/acc/dataset.csv"}
CLUSTER_USER="vmuser"
PROJECT_DIR="~/aac-practicas"

# Lista de todos los nodos del cluster
NODOS=("AAC15-1" "AAC15-2" "AAC15-3" "AAC15-4")
NODO_MAESTRO="AAC15-1"

echo "=================================================================="
echo "  DESPLIEGUE AUTOMÁTICO EN CLUSTER AAC15"
echo "=================================================================="
echo "Nodos: ${NODOS[*]}"
echo "Procesos: $NUM_PROCESOS"
echo "Archivo: $ARCHIVO_CSV"
echo "=================================================================="

# Función para verificar conectividad
check_connectivity() {
    echo " Verificando conectividad con todos los nodos..."
    
    for nodo in "${NODOS[@]}"; do
        echo -n "  $nodo... "
        if ssh -o ConnectTimeout=5 -o BatchMode=yes $CLUSTER_USER@$nodo "echo 'OK' " &>/dev/null; then
            echo "Conectado"
        else
            echo " FALLÓ"
            return 1
        fi
    done
    return 0
}

# Función para instalar dependencias en todos los nodos
install_dependencies() {
    echo "Instalando dependencias en todos los nodos..."
    
    for nodo in "${NODOS[@]}"; do
        echo "  Instalando en $nodo..."
        ssh $CLUSTER_USER@$nodo "
            echo ' Actualizando paquetes...'
            sudo apt update > /dev/null 2>&1
            
            echo ' Instalando compilador y herramientas...'
            sudo apt install -y build-essential g++ gcc cmake > /dev/null 2>&1
            
            echo ' Instalando librerías científicas...'
            sudo apt install -y libglm-dev > /dev/null 2>&1
            
            echo ' Instalando MPI...'
            sudo apt install -y openmpi-bin libopenmpi-dev > /dev/null 2>&1
            
            echo ' $nodo: Dependencias instaladas'
        " &
    done
    
    # Esperar a que todas las instalaciones terminen
    wait
    echo " Todas las dependencias instaladas"
}

# Función para transferir proyecto a todos los nodos
transfer_to_all_nodes() {
    echo " Transferiendo proyecto a todos los nodos..."
    
    for nodo in "${NODOS[@]}"; do
        echo "  Transferiendo a $nodo..."
        
        # Crear estructura de directorios
        ssh $CLUSTER_USER@$nodo "mkdir -p $PROJECT_DIR/{src,data,build,scripts,results,backups}"
        
        # Transferir código fuente
        scp -r ../libDomain $CLUSTER_USER@$nodo:$PROJECT_DIR/ > /dev/null 2>&1
        scp -r ../computeDistributed $CLUSTER_USER@$nodo:$PROJECT_DIR/ > /dev/null 2>&1
        scp -r ../computeShared $CLUSTER_USER@$nodo:$PROJECT_DIR/ > /dev/null 2>&1
        scp ../CMakeLists.txt $CLUSTER_USER@$nodo:$PROJECT_DIR/ > /dev/null 2>&1
        
        # Transferir scripts
        scp build_cluster.sh run_mpi_cluster.sh $CLUSTER_USER@$nodo:$PROJECT_DIR/scripts/ > /dev/null 2>&1
        
        echo "    OK  $nodo: Proyecto transferido"
    done &
    
    # En paralelo, buscar y transferir dataset al nodo maestro
    echo "  Buscando dataset..."
    DATASET_PATHS=(
        "$HOME/Documentos/ARQUITECTURA AVANZADA DE COMPUTADORES/data/dataset.csv"
        "$HOME/Descargas/dataset.csv"
        "$HOME/dataset.csv"
        "./data/dataset.csv"
        "../data/dataset.csv"
        "/home/fernando/dataset.csv"
    )
    
    DATASET_FOUND=""
    for path in "${DATASET_PATHS[@]}"; do
        if [ -f "$path" ]; then
            DATASET_FOUND="$path"
            echo "    OK  Dataset encontrado: $path"
            break
        fi
    done
    
    if [ -n "$DATASET_FOUND" ]; then
        scp "$DATASET_FOUND" $CLUSTER_USER@$NODO_MAESTRO:$PROJECT_DIR/data/ > /dev/null 2>&1
        echo "    OK Dataset transferido a $NODO_MAESTRO"
    else
        echo "    ERROR  Dataset no encontrado. Transferir manualmente:"
        echo "       scp /ruta/al/dataset.csv $CLUSTER_USER@$NODO_MAESTRO:$PROJECT_DIR/data/"
    fi
    
    wait
    echo "OK Transferencia completada a todos los nodos"
}

# Función para compilar en todos los nodos
compile_on_all_nodes() {
    echo "... Compilando en todos los nodos (en paralelo)..."
    
    for nodo in "${NODOS[@]}"; do
        echo "  Compilando en $nodo..."
        ssh $CLUSTER_USER@$nodo "
            cd $PROJECT_DIR
            if [ -f scripts/build_cluster.sh ]; then
                chmod +x scripts/build_cluster.sh
                ./scripts/build_cluster.sh > build_log_$nodo.txt 2>&1
                if [ \$? -eq 0 ]; then
                    echo '    OK $nodo: Compilación exitosa'
                else
                    echo '    ERROR $nodo: Error en compilación. Ver build_log_$nodo.txt'
                fi
            else
                echo '    ERROR $nodo: Script build_cluster.sh no encontrado'
            fi
        " &
    done
    
    # Esperar a que todas las compilaciones terminen
    wait
    echo "OK Compilación en todos los nodos completada"
}

# Función para verificar ejecutables en todos los nodos
verify_executables() {
    echo "... Verificando ejecutables en todos los nodos..."
    
    for nodo in "${NODOS[@]}"; do
        echo -n "  $nodo... "
        if ssh $CLUSTER_USER@$nodo "[ -f $PROJECT_DIR/build/computeDistributed/computeDistributed ]"; then
            echo "OK computeDistributed"
        else
            echo "ERROR FALTANTE"
        fi
    done
}

# Función para crear hostfile de MPI
create_mpi_hostfile() {
    echo "  Creando archivo de configuración MPI..."
    
    # Crear hostfile en el nodo maestro
    ssh $CLUSTER_USER@$NODO_MAESTRO "cd $PROJECT_DIR && cat > mpi_hosts << 'EOF'
# Configuración del Cluster AAC15 para MPI
# Nodos y número de slots (procesos) por nodo

AAC15-1 slots=4
AAC15-2 slots=4  
AAC15-3 slots=4
AAC15-4 slots=4

# Configuración alternativa para diferentes cargas:
# AAC15-1 slots=2
# AAC15-2 slots=2
# AAC15-3 slots=2
# AAC15-4 slots=2
EOF"

    echo "OK Archivo mpi_hosts creado en $NODO_MAESTRO"
}

# Función para ejecutar en el cluster
execute_on_cluster() {
    echo "   EJECUTANDO EN EL CLUSTER..."
    echo "   Nodos: ${NODOS[*]}"
    echo "   Procesos: $NUM_PROCESOS"
    echo "   Archivo: $ARCHIVO_CSV"
    
    # Ejecutar con MPI
    ssh $CLUSTER_USER@$NODO_MAESTRO "
        cd $PROJECT_DIR
        echo '.. Iniciando ejecución MPI...'
        echo '================================'
        
        mpirun -np $NUM_PROCESOS --hostfile mpi_hosts --display-allocation \
            ./build/computeDistributed/computeDistributed \"$ARCHIVO_CSV\"
        
        EXIT_CODE=\$?
        echo '================================'
        if [ \$EXIT_CODE -eq 0 ]; then
            echo 'OK Ejecución completada exitosamente'
        else
            echo 'ER Error en ejecución (Código: '\$EXIT_CODE')'
        fi
        exit \$EXIT_CODE
    "
}

# Función para monitorear estado del cluster
cluster_status() {
    echo ".. ESTADO ACTUAL DEL CLUSTER:"
    
    for nodo in "${NODOS[@]}"; do
        echo "  $nodo:"
        ssh $CLUSTER_USER@$nodo "
            echo -n '    CPU: ' && uptime | awk '{print \$10 \$11 \$12}'
            echo -n '    Memoria: ' && free -h | awk 'NR==2{print \$3\"/\"\$2}'
            echo -n '    Disco: ' && df -h / | awk 'NR==2{print \$5}'
            echo -n '    MPI: ' && which mpirun > /dev/null && echo 'OK' || echo 'ERROR'
            echo -n '    Ejecutable: ' && [ -f $PROJECT_DIR/build/computeDistributed/computeDistributed ] && echo 'OK' || echo 'ERROR'
        " 2>/dev/null || echo "    ERR  No se pudo conectar"
    done
}

# ====================================================================
# EJECUCIÓN PRINCIPAL
# ====================================================================

# Paso 1: Verificar conectividad
if ! check_connectivity; then
    echo "ERROR Error: No se puede conectar a todos los nodos"
    echo "   Verifica la configuración SSH y la conexión a la VPN"
    exit 1
fi

# Paso 2: Mostrar estado actual
cluster_status

# Paso 3: Preguntar si instalar dependencias
read -p "¿Instalar dependencias en todos los nodos? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    install_dependencies
fi

# Paso 4: Transferir proyecto
read -p "¿Transferir proyecto a todos los nodos? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    transfer_to_all_nodes
fi

# Paso 5: Compilar
read -p "¿Compilar en todos los nodos? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    compile_on_all_nodes
    verify_executables
fi

# Paso 6: Crear configuración MPI
create_mpi_hostfile

# Paso 7: Ejecutar
read -p "¿Ejecutar en el cluster? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    execute_on_cluster
fi

echo ""
echo "=================================================================="
echo "   PROCESO COMPLETADO"
echo "=================================================================="
echo "Comandos manuales útiles:"
echo "  Compilar en un nodo específico:"
echo "    ssh vmuser@AAC15-1 'cd ~/aac-practicas && ./scripts/build_cluster.sh'"
echo ""
echo "  Ejecutar con diferente número de procesos:"
echo "    ssh vmuser@AAC15-1 'cd ~/aac-practicas && mpirun -np 8 --hostfile mpi_hosts ./build/computeDistributed/computeDistributed data/dataset.csv'"
echo ""
echo "  Ver logs de compilación:"
echo "    ssh vmuser@AAC15-1 'cat ~/aac-practicas/build_log_AAC15-1.txt'"
echo "=================================================================="
