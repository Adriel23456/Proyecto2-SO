#!/bin/bash
# run_mpi_safe.sh - Lanzador simple MPI para OpenMPI (1 proceso por nodo)

# ===== Configuración OpenMPI =====
MPIRUN="/opt/openmpi-4.1.6/bin/mpirun"

# ===== Ubicaciones de tu proyecto =====
APP_DIR="$HOME/Documents/Proyecto2-SO/MainSystem"
EXECUTABLE="$APP_DIR/main"

# Hostfile original (solo para referencia, ya NO se pasa directo a mpirun)
ORIGINAL_HOSTFILE="$HOME/.mpi_hostfile"

# Hosts candidatos (los que quieres usar en el cluster)
HOSTS=(localhost slave1 slave2 slave3)

# ===== Argumentos a pasar al main =====
EXTRA_ARGS=("$@")

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
echo "Hostfile original    : $ORIGINAL_HOSTFILE (solo referencia)"
echo "MPIRUN               : $MPIRUN"
echo "Args para main       : ${EXTRA_ARGS[*]:-(ninguno)}"
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

# El hostfile original es opcional ahora, pero si existe lo mostramos
if [ ! -f "$ORIGINAL_HOSTFILE" ]; then
    echo -e "${YELLOW}Aviso:${NC} no se encontró ~/.mpi_hostfile (no es crítico para este script)"
fi

# ===== Verificar nodos y construir hostfile filtrado =====
TEMP_HOSTFILE=$(mktemp /tmp/mpi_hosts_XXXXXX)
AVAILABLE_HOSTS=()
available_count=0

# Detectar número de cores por nodo
CORES_PER_NODE=4  # O usa: $(nproc) para detección automática

# Construir hostfile con múltiples slots
for host in "${HOSTS[@]}"; do
    printf "  %s: " "$host"
    if ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "ls -l '$EXECUTABLE'" >/dev/null 2>&1; then
        echo -e "${GREEN}OK${NC}"
        AVAILABLE_HOSTS+=("$host")
        # ⭐ CAMBIO CRÍTICO: Múltiples slots por nodo
        echo "$host slots=$CORES_PER_NODE" >> "$TEMP_HOSTFILE"
        available_count=$((available_count + CORES_PER_NODE))
    else
        echo -e "${YELLOW}No encontrado${NC}"
    fi
done

# Actualizar número total de procesos
NPROCS="$available_count"

echo
echo "Nodos disponibles con '$EXECUTABLE': $available_count"

if [ "$available_count" -eq 0 ]; then
    echo -e "${RED}ERROR:${NC} no hay nodos con el ejecutable disponible. Abortando."
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# ===== 1 proceso por nodo disponible =====
NPROCS="$available_count"
echo "Procesos totales a lanzar (-np): $NPROCS"
echo

# Mostrar hostfile efectivo que SÍ usará mpirun
echo -e "${YELLOW}Hostfile efectivo (filtrado):${NC}"
nl -ba "$TEMP_HOSTFILE"
echo

# ===== Cambiar al directorio de la app =====
cd "$APP_DIR" || {
    echo -e "${RED}ERROR:${NC} no se pudo hacer cd a $APP_DIR"
    rm -f "$TEMP_HOSTFILE"
    exit 1
}

# ===== Ejecutar mpirun con hostfile filtrado =====
echo "▶ Ejecutando mpirun..."
echo "$MPIRUN -np $NPROCS --hostfile $TEMP_HOSTFILE --map-by node --bind-to core --report-bindings -x PATH -x LD_LIBRARY_PATH ./main ${EXTRA_ARGS[*]}"
echo

# Cambiar opciones de mpirun para mejor distribución
"$MPIRUN" \
    -np "$NPROCS" \
    --hostfile "$TEMP_HOSTFILE" \
    --map-by slot \              # Cambio: slot en lugar de node
    --bind-to core \
    --oversubscribe \            # Permite más procesos si es necesario
    --report-bindings \
    -x PATH \
    -x LD_LIBRARY_PATH \
    ./main "${EXTRA_ARGS[@]}"

EXIT_CODE=$?

# Limpiar hostfile temporal
rm -f "$TEMP_HOSTFILE"

echo
echo "==========================================="
echo " mpirun terminó con código: $EXIT_CODE"
echo "==========================================="

exit "$EXIT_CODE"
