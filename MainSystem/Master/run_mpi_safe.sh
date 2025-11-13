#!/bin/bash
# run_mpi_safe.sh - Lanzador MPI optimizado para múltiples cores

# ===== Configuración OpenMPI =====
MPIRUN="/opt/openmpi-4.1.6/bin/mpirun"

# ===== Ubicaciones de tu proyecto =====
APP_DIR="$HOME/Documents/Proyecto2-SO/MainSystem"
EXECUTABLE="$APP_DIR/main"

# Hosts candidatos
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

# ===== Verificar nodos y construir hostfile filtrado =====
TEMP_HOSTFILE=$(mktemp /tmp/mpi_hosts_XXXXXX)
AVAILABLE_HOSTS=()
total_slots=0

echo "▶ Verificando nodos y ejecutable:"
for host in "${HOSTS[@]}"; do
    printf "  %s: " "$host"
    if ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "ls -l '$EXECUTABLE'" >/dev/null 2>&1; then
        echo -e "${GREEN}OK${NC}"
        AVAILABLE_HOSTS+=("$host")
        
        # Detectar número de cores disponibles en cada nodo
        cores=$(ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "nproc" 2>/dev/null)
        
        # Usar la mitad de los cores disponibles (para balance)
        # O puedes usar todos cambiando a: slots=$cores
        slots=$((cores / 2))
        if [ "$slots" -lt 1 ]; then
            slots=1
        fi
        
        echo "    Cores detectados: $cores, usando slots: $slots"
        echo "$host slots=$slots" >> "$TEMP_HOSTFILE"
        total_slots=$((total_slots + slots))
    else
        echo -e "${YELLOW}No encontrado${NC}"
    fi
done

echo
echo "Slots totales disponibles: $total_slots"

if [ "$total_slots" -eq 0 ]; then
    echo -e "${RED}ERROR:${NC} no hay nodos con el ejecutable disponible. Abortando."
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# Usar todos los slots disponibles
NPROCS="$total_slots"
echo "Procesos totales a lanzar (-np): $NPROCS"
echo

echo -e "${YELLOW}Hostfile efectivo (filtrado):${NC}"
cat "$TEMP_HOSTFILE"
echo

# ===== Cambiar al directorio de la app =====
cd "$APP_DIR" || {
    echo -e "${RED}ERROR:${NC} no se pudo hacer cd a $APP_DIR"
    rm -f "$TEMP_HOSTFILE"
    exit 1
}

# ===== Ejecutar mpirun (CORREGIDO - sin backslashes problemáticos) =====
echo "▶ Ejecutando mpirun..."
echo "$MPIRUN -np $NPROCS --hostfile $TEMP_HOSTFILE --map-by slot --bind-to core --report-bindings -x PATH -x LD_LIBRARY_PATH ./main ${EXTRA_ARGS[*]}"
echo

"$MPIRUN" -np "$NPROCS" --hostfile "$TEMP_HOSTFILE" --map-by slot --bind-to core --report-bindings -x PATH -x LD_LIBRARY_PATH ./main "${EXTRA_ARGS[@]}"

EXIT_CODE=$?

# Limpiar hostfile temporal
rm -f "$TEMP_HOSTFILE"

echo
echo "==========================================="
echo " mpirun terminó con código: $EXIT_CODE"
echo "==========================================="

exit "$EXIT_CODE"