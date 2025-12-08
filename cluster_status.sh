#!/bin/bash

#Monitoreo del estado del cluster
echo "Estado del Cluster AAC15"
echo "------------------------"

for node in AAC15-1 AAC15-2 AAC15-3 AAC15-4; do
    echo "Monitoreo : $node:"
    ssh "$node" "
        echo '  CPU: ' && uptime | awk '{print \$10 \$11 \$12}'
        echo '  Memoria: ' && free -h | awk 'NR==2{print \$3\"/\"\$2}'
        echo '  Disco: ' && df -h / | awk 'NR==2{print \$5}'
        echo '  Procesos: ' && ps aux | wc -l
    " 2>/dev/null || echo "   No disponible"
    echo ""
done
