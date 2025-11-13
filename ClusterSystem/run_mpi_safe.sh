#!/bin/bash
# Script: mpirun-safe
# Ejecuta MPI solo con nodos disponibles en clúster heterogéneo

# ===== Configuración =====
EXECUTABLE="./ejemplo"
HOSTFILE="$HOME/.mpi_hostfile"
TEMP_HOSTFILE="/tmp/mpi_hostfile_available_$$"
TIMEOUT=3
MPI_PATH="/opt/openmpi-4.1.6"
DEFAULT_SLOTS=2

# ===== Colores =====
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ===== Funciones =====
check_node() {
    local host=$1
    local timeout=$2
    ssh -o ConnectTimeout=$timeout \
        -o BatchMode=yes \
        -o StrictHostKeyChecking=no \
        -o LogLevel=ERROR \
        "$host" "exit" 2>/dev/null
    return $?
}

get_arch() {
    local host=$1
    ssh -o ConnectTimeout=2 \
        -o BatchMode=yes \
        -o LogLevel=ERROR \
        "$host" "uname -m" 2>/dev/null || echo "unknown"
}

# ===== Procesar argumentos =====
while [[ $# -gt 0 ]]; do
    case $1 in
        -e|--executable)
            EXECUTABLE="$2"
            shift 2
            ;;
        -n|--nprocs)
            FORCE_NPROCS="$2"
            shift 2
            ;;
        -h|--help)
            echo "Uso: $0 [opciones]"
            echo "  -e, --executable PROG   Ejecutable MPI (default: ./ejemplo)"
            echo "  -n, --nprocs N          Forzar número de procesos"
            echo "  -h, --help              Mostrar esta ayuda"
            exit 0
            ;;
        *)
            echo "Opción desconocida: $1"
            exit 1
            ;;
    esac
done

# ===== Header =====
echo -e "${BLUE}╔══════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║    ${YELLOW}Sistema MPI Heterogéneo OpenMPI 4.1.6${BLUE}    ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════╝${NC}\n"

# ===== Validaciones =====
if [ ! -f "$HOSTFILE" ]; then
    echo -e "${RED}✗ ERROR: No se encuentra $HOSTFILE${NC}"
    exit 1
fi

if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}✗ ERROR: No se encuentra el ejecutable $EXECUTABLE${NC}"
    exit 1
fi

if [ ! -x "$MPI_PATH/bin/mpirun" ]; then
    echo -e "${RED}✗ ERROR: OpenMPI no está instalado en $MPI_PATH${NC}"
    exit 1
fi

# ===== Escaneo de clúster =====
echo -e "${YELLOW}▶ Escaneando clúster...${NC}\n"

: > "$TEMP_HOSTFILE"
available_nodes=0
total_slots=0
configured_nodes=0

# Leer hostfile línea por línea
while IFS= read -r line; do
    # Saltar vacías y comentarios (líneas que empiezan con #)
    case "$line" in
        ''|\#*) continue ;;
    esac

    # Partir la línea en campos: host es el primer campo
    set -- $line
    host="$1"
    slots="$DEFAULT_SLOTS"

    # Buscar slots=N en el resto de campos
    shift
    for tok in "$@"; do
        case "$tok" in
            slots=*)
                slots="${tok#slots=}"
                ;;
        esac
    done

    configured_nodes=$((configured_nodes + 1))

    printf "  Verificando %-20s " "$host"

    if check_node "$host" "$TIMEOUT"; then
        arch=$(get_arch "$host")
        echo -e "${GREEN}✓${NC} ONLINE [${arch}] (slots=${slots})"
        echo "$host slots=$slots" >> "$TEMP_HOSTFILE"
        available_nodes=$((available_nodes + 1))
        total_slots=$((total_slots + slots))
    else
        echo -e "${RED}✗${NC} OFFLINE"
    fi
done < "$HOSTFILE"

# ===== Resumen =====
echo -e "\n${YELLOW}▶ Resumen del clúster:${NC}"
echo "  • Nodos configurados: $configured_nodes"
echo "  • Nodos disponibles:  $available_nodes"
echo "  • Slots totales:      $total_slots"

if [ "$available_nodes" -eq 0 ]; then
    echo -e "\n${RED}✗ ERROR: No hay nodos disponibles${NC}"
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# ===== Determinar NPROCS =====
if [ -n "${FORCE_NPROCS:-}" ]; then
    NPROCS=$FORCE_NPROCS
    if [ "$NPROCS" -gt "$total_slots" ]; then
        echo -e "\n${YELLOW}⚠ Advertencia: Solicitaste $NPROCS procesos pero solo hay $total_slots slots${NC}"
        echo -e "  Ajustando a $total_slots procesos..."
        NPROCS=$total_slots
    fi
else
    NPROCS=$total_slots
fi

# ===== Ejecutar MPI =====
echo -e "\n${BLUE}▶ Ejecutando MPI con $NPROCS procesos en $available_nodes nodos${NC}\n"
echo -e "${YELLOW}════════════════════════════════════════════════${NC}\n"

"$MPI_PATH/bin/mpirun" \
    --hostfile "$TEMP_HOSTFILE" \
    --np "$NPROCS" \
    --map-by node \
    --bind-to core \
    --report-bindings \
    -x PATH \
    -x LD_LIBRARY_PATH \
    "$EXECUTABLE"

EXIT_CODE=$?

echo -e "\n${YELLOW}════════════════════════════════════════════════${NC}"

rm -f "$TEMP_HOSTFILE"

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "\n${GREEN}✓ Ejecución completada exitosamente${NC}\n"
else
    echo -e "\n${RED}✗ Error en la ejecución (código: $EXIT_CODE)${NC}\n"
fi

exit $EXIT_CODE
