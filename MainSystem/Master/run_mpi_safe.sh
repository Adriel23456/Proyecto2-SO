#!/bin/bash
# run_mpi_safe.sh - Lanzador MPI con OpenMP (1 proceso por nodo, múltiples threads)

# ===== Configuración OpenMPI =====
MPIRUN="/opt/openmpi-4.1.6/bin/mpirun"

# ===== Ubicaciones de tu proyecto =====
APP_DIR="$HOME/Documents/Proyecto2-SO/MainSystem"
EXECUTABLE="$APP_DIR/main"

# Hostfile original (solo para referencia)
ORIGINAL_HOSTFILE="$HOME/.mpi_hostfile"

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
echo "  run_mpi_safe.sh - OpenMPI + OpenMP"
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

if [ ! -f "$ORIGINAL_HOSTFILE" ]; then
    echo -e "${YELLOW}Aviso:${NC} no se encontró ~/.mpi_hostfile (no es crítico)"
fi

# ===== Verificar nodos y construir hostfile filtrado =====
TEMP_HOSTFILE=$(mktemp /tmp/mpi_hosts_XXXXXX)
AVAILABLE_HOSTS=()
available_count=0

echo "▶ Verificando nodos y ejecutable:"
for host in "${HOSTS[@]}"; do
    printf "  %s: " "$host"
    if ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "ls -l '$EXECUTABLE'" >/dev/null 2>&1; then
        # Detectar número de cores disponibles
        cores=$(ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "nproc" 2>/dev/null || echo "4")
        echo -e "${GREEN}OK${NC} ($cores cores)"
        AVAILABLE_HOSTS+=("$host")
        available_count=$((available_count + 1))
        # 1 proceso por nodo (pero usará múltiples threads)
        echo "$host slots=1" >> "$TEMP_HOSTFILE"
    else
        echo -e "${YELLOW}No encontrado${NC}"
    fi
done

echo
echo "Nodos disponibles con '$EXECUTABLE': $available_count"

if [ "$available_count" -eq 0 ]; then
    echo -e "${RED}ERROR:${NC} no hay nodos con el ejecutable disponible. Abortando."
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# ===== Configurar OpenMP =====
# Detectar cores del nodo local para configurar threads
LOCAL_CORES=$(nproc)
# Usar todos los cores disponibles (o ajusta según necesites)
export OMP_NUM_THREADS=$LOCAL_CORES
export OMP_PROC_BIND=true
export OMP_PLACES=cores

echo
echo "═══════════════════════════════════════════════════════════"
echo "  CONFIGURACIÓN OpenMP:"
echo "  - Threads por proceso: $OMP_NUM_THREADS"
echo "  - Binding: $OMP_PROC_BIND"
echo "  - Places: $OMP_PLACES"
echo "═══════════════════════════════════════════════════════════"
echo

# ===== 1 proceso por nodo disponible =====
NPROCS="$available_count"
echo "Procesos MPI totales a lanzar (-np): $NPROCS"
echo

echo -e "${YELLOW}Hostfile efectivo (filtrado):${NC}"
nl -ba "$TEMP_HOSTFILE"
echo

# ===== Cambiar al directorio de la app =====
cd "$APP_DIR" || {
    echo -e "${RED}ERROR:${NC} no se pudo hacer cd a $APP_DIR"
    rm -f "$TEMP_HOSTFILE"
    exit 1
}

# ===== FIX: DISABLE PMIx COMPRESSION =====
export OMPI_MCA_pmix_compress=0
export OMPI_MCA_pcompress_base_suppress=1

echo
echo "📌 Desactivando compresión PMIx (fix heterogéneo)"
echo "   OMPI_MCA_pmix_compress=0"
echo "   OMPI_MCA_pcompress_base_suppress=1"
echo

# ===== Ejecutar mpirun con variables OpenMP =====
echo "▶ Ejecutando mpirun con OpenMP..."
echo "$MPIRUN -np $NPROCS --hostfile $TEMP_HOSTFILE --map-by node --bind-to socket --report-bindings --mca pmix_compress 0 --mca pcompress_base_suppress 1 -x OMP_NUM_THREADS -x OMP_PROC_BIND -x OMP_PLACES -x PATH -x LD_LIBRARY_PATH ./main ${EXTRA_ARGS[*]}"
echo

"$MPIRUN" \
    -np "$NPROCS" \
    --hostfile "$TEMP_HOSTFILE" \
    --map-by node \
    --bind-to socket \
    --report-bindings \
    -x OMP_NUM_THREADS \
    -x OMP_PROC_BIND \
    -x OMP_PLACES \
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