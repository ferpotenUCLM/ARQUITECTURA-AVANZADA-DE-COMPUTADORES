#!/bin/bash

# Script de automatización para el cluster AAC15
# Este script configura el acceso SSH y prueba MPI en todos los nodos

echo "=== Configuración Automática del Cluster AAC15 ==="

# Configurar SSH en local (tu máquina)
echo "1. Configurando SSH en máquina local..."
cat > ~/.ssh/config << 'EOF'
# Configuración Cluster AAC15-Configuración para los 4 nodos del cluster
Host AAC15-public
    Hostname 10.100.139.198
    User vmuser
    IdentityFile ~/.ssh/AAC15.key


Host AAC15-1
    Hostname 192.168.117.51
    User vmuser
    ProxyJump AAC15-public
    IdentityFile ~/.ssh/AAC15.key

Host AAC15-2
    Hostname 192.168.117.217
    User vmuser
    ProxyJump AAC15-public
    IdentityFile ~/.ssh/AAC15.key

Host AAC15-3
    Hostname 192.168.117.157
    User vmuser
    ProxyJump AAC15-public
    IdentityFile ~/.ssh/AAC15.key

Host AAC15-4
    Hostname 192.168.117.154
    User vmuser
    ProxyJump AAC15-public
    IdentityFile ~/.ssh/AAC15.key
EOF

echo "Configuración SSH local completada"

# Función para ejecutar comandos en todos los nodos
execute_on_all_nodes() {
    local command="$1"
    local description="$2"

    echo ""
    echo "$description"
    echo "────────────────────────────────────"

    for node in AAC15-1 AAC15-2 AAC15-3 AAC15-4; do
        echo "Ejecutando en $node..."
        if ssh -o ConnectTimeout=10 "$node" "$command"; then
            echo "$node: OK"
        else
            echo "$node: Error"
        fi
        echo ""
    done
}

# Instalar OpenMPI en todos los nodos
echo ""
echo "2. Instalando OpenMPI en todos los nodos..."
execute_on_all_nodes "sudo apt update && sudo apt install -y openmpi-bin libopenmpi-dev" "Instalando OpenMPI"

# Probar MPI
echo ""
echo "3. Probando MPI en el cluster..."
echo "Ejecutando: mpirun --host AAC15-1,AAC15-2,AAC15-3,AAC15-4 hostname"
ssh AAC15-public "mpirun --host AAC15-1,AAC15-2,AAC15-3,AAC15-4 hostname"

# Verificar instalación
echo ""
echo "4. Verificando instalación en todos los nodos..."
execute_on_all_nodes "which mpirun && mpirun --version | head -n 1" "Verificando MPI"

# Mostrar información del cluster
echo ""
echo "5. Información del cluster:"
echo "------------------------------"
for node in AAC15-1 AAC15-2 AAC15-3 AAC15-4; do
    echo "$node:"
    ssh "$node" "hostname && echo 'IP: ' && hostname -I | cut -d' ' -f1"
    echo ""
done

echo "=== Configuración completada ==="
echo ""
echo "Comandos útiles:"
echo "  ssh AAC15-1                    # Conectar al nodo 1"
echo "  ssh AAC15-2                    # Conectar al nodo 2"
echo "  ssh AAC15-3                    # Conectar al nodo 3"
echo "  ssh AAC15-4                    # Conectar al nodo 4"
echo "  mpirun --host AAC15-1,AAC15-2,AAC15-3,AAC15-4 hostname  # Probar MPI"
