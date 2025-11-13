#!/bin/bash
# run_mpi_safe.sh - Lanzador simple MPI para OpenMPI (1 proceso por nodo)

# ===== Configuración OpenMPI =====
MPIRUN="/opt/openmpi-4.1.6/bin/mpirun"

# ===== Ubicaciones de tu proyecto =====
APP_DIR="$HOME/Documents/Proyecto2-SO/MainSystem"
EXECUTABLE="$APP_DIR/main"

# Hostfile que usará mpirun
HOSTFILE="$HOME/.mpi_hostfile"

# Hosts que queremos considerar (master + slaves)
HOSTS=(localhost slave1 slave2 slave3)

# ===== Colores =====
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "==========================================="
echo "  run_mpi_safe.sh - OpenMPI (MainSystem)"
echo "==========================================="
echo
echo "Directorio de la app : $APP_DIR"
echo "Ejecutable           : $EXECUTABLE"
echo "Hostfile             : $HOSTFILE"
echo "MPIRUN               : $MPIRUN"
echo

# ===== Validaciones básicas =====
if [ ! -x "$MPIRUN" ]; then
    echo -e "${RED}ERROR:${NC} no se encontró mpirun en: $MPIRUN"
    exit 1
fi

if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}ERROR:${NC} no se encontró el ejecutable: $EXECUTABLE"
    exit 1
fi

if [ ! -f "$HOSTFILE" ]; then
    echo -e "${RED}ERROR:${NC} no se encontró el hostfile: $HOSTFILE"
    exit 1
fi

# ===== Verificar nodos y ejecutable =====
available_hosts=0
echo "▶ Verificando nodos y ejecutable:"
for host in "${HOSTS[@]}"; do
    printf "  %s: " "$host"
    if ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "ls -l '$EXECUTABLE'" >/dev/null 2>&1; then
        echo -e "${GREEN}OK${NC}"
        available_hosts=$((available_hosts + 1))
    else
        echo -e "${YELLOW}No encontrado${NC}"
    fi
done

echo
echo "Nodos disponibles con '$EXECUTABLE': $available_hosts"

if [ "$available_hosts" -eq 0 ]; then
    echo -e "${RED}ERROR:${NC} no hay nodos con el ejecutable disponible. Abortando."
    exit 1
fi

# ===== 1 proceso por nodo disponible =====
NPROCS="$available_hosts"
echo "Procesos totales a lanzar (-np): $NPROCS"
echo

# ===== Cambiar al directorio de la app =====
cd "$APP_DIR" || {
    echo -e "${RED}ERROR:${NC} no se pudo hacer cd a $APP_DIR"
    exit 1
}

# ===== Ejecutar mpirun =====
echo "▶ Ejecutando mpirun..."
echo "$MPIRUN -np $NPROCS --hostfile $HOSTFILE --map-by node --bind-to core --report-bindings -x PATH -x LD_LIBRARY_PATH ./main"
echo

"$MPIRUN" \
    -np "$NPROCS" \
    --hostfile "$HOSTFILE" \
    --map-by node \
    --bind-to core \
    --report-bindings \
    -x PATH \
    -x LD_LIBRARY_PATH \
    ./main

EXIT_CODE=$?

echo
echo "==========================================="
echo " mpirun terminó con código: $EXIT_CODE"
echo "==========================================="

exit "$EXIT_CODE"
