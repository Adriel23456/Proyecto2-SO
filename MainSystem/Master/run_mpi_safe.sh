#!/bin/bash
#==============================================================================
# Script: run_mpi_safe.sh (MASTER)
# Ejecuta el sistema de procesamiento distribuido de imágenes
# Automáticamente detecta slaves disponibles y ejecuta con ellos
#==============================================================================

# ===== Configuración =====
EXECUTABLE="$HOME/Documents/Proyecto2-SO/MainSystem/main"
ORIGINAL_HOSTFILE="${HYDRA_HOST_FILE:-$HOME/.mpi_hostfile}"
TEMP_HOSTFILE="/tmp/.mpi_hostfile_available"
TIMEOUT=3
IFACE=""
IMAGE_FILE=""

# ===== Colores =====
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ===== Función de ayuda =====
show_help() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  Sistema de Procesamiento Distribuido de Imágenes${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo "Uso: $0 <imagen.png> [-t TIMEOUT] [-i IFACE]"
    echo ""
    echo "Parámetros:"
    echo "  <imagen.png>  - Ruta de la imagen a procesar (requerido)"
    echo "  -t TIMEOUT    - Timeout para verificar slaves (default: 3)"
    echo "  -i IFACE      - Interfaz de red a usar (opcional)"
    echo ""
    echo "Ejemplos:"
    echo "  $0 image.png"
    echo "  $0 ~/Pictures/photo.png -t 5"
    echo "  $0 image.png -i 192.168.18.242"
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    exit 1
}

# ===== Procesar argumentos =====
if [ $# -lt 1 ]; then
    echo -e "${RED}ERROR: Falta argumento de imagen${NC}"
    show_help
fi

IMAGE_FILE="$1"
shift

# Procesar opciones adicionales
while getopts ":t:i:h" opt; do
    case "$opt" in
        t) TIMEOUT="$OPTARG" ;;
        i) IFACE="$OPTARG" ;;
        h) show_help ;;
        \?) echo -e "${RED}Opción inválida: -$OPTARG${NC}"; show_help ;;
    esac
done

# ===== Validaciones =====
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}  Validando configuración...${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Verificar que la imagen existe
if [ ! -f "$IMAGE_FILE" ]; then
    echo -e "${RED}ERROR: Archivo de imagen no encontrado: $IMAGE_FILE${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} Imagen encontrada: $IMAGE_FILE"

# Verificar hostfile
if [ ! -f "$ORIGINAL_HOSTFILE" ]; then
    echo -e "${RED}ERROR: No se encuentra $ORIGINAL_HOSTFILE${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} Hostfile encontrado: $ORIGINAL_HOSTFILE"

# Verificar ejecutable
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}ERROR: No se encuentra el ejecutable: $EXECUTABLE${NC}"
    echo ""
    echo "Compila primero con:"
    echo "  cd ~/Documents/Proyecto2-SO/MainSystem/Master"
    echo "  make"
    exit 1
fi
echo -e "${GREEN}✓${NC} Ejecutable encontrado: $EXECUTABLE"

echo ""

# ===== Verificar disponibilidad de slaves =====
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}  Verificando disponibilidad de slaves...${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo ""

: > "$TEMP_HOSTFILE"
available_hosts=0
available_slots=0
configured_hosts=0

while IFS= read -r raw <&3 || [ -n "$raw" ]; do
    line="$(echo "$raw" | sed -E 's/^[[:space:]]+//; s/[[:space:]]+$//')"
    [[ -z "$line" || "$line" =~ ^# ]] && continue
    line="$(echo "$line" | sed -E 's/[[:space:]]*#.*$//')"
    [[ -z "$line" ]] && continue

    host="$(echo "$line" | cut -d':' -f1 | xargs)"
    ppn="$(echo "$line" | awk -F: 'NF>=2{print $2; exit} NF<2{print ""}')"
    [[ -z "$ppn" ]] && ppn=1

    if ! [[ "$ppn" =~ ^[0-9]+$ ]] || [ "$ppn" -lt 1 ]; then
        ppn=1
    fi

    configured_hosts=$((configured_hosts+1))
    printf "  Verificando %-18s (ppn=%-2s) ... " "$host" "$ppn"

    ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "exit" < /dev/null 2>/dev/null
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}DISPONIBLE${NC}"
        echo "${host}:${ppn}" >> "$TEMP_HOSTFILE"
        available_hosts=$((available_hosts+1))
        available_slots=$((available_slots+ppn))
    else
        echo -e "${RED}NO DISPONIBLE${NC}"
    fi
done 3< "$ORIGINAL_HOSTFILE"

echo ""
echo -e "${YELLOW}Resumen:${NC}"
echo "  Hosts configurados : $configured_hosts"
echo "  Hosts disponibles   : $available_hosts"
echo "  Slots disponibles   : $available_slots"
echo ""

if [ "$available_hosts" -eq 0 ]; then
    echo -e "${RED}ERROR: No hay slaves disponibles. Abortando.${NC}"
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# ===== Ejecutar MPI =====
NPROCS=$((available_slots + 1))  # +1 para el master

echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  Ejecutando procesamiento distribuido...${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo ""
echo "  Procesos totales: $NPROCS (1 master + $available_slots slaves)"
echo "  Imagen: $IMAGE_FILE"
echo ""

MPIEXEC="/usr/local/mpich-4.3.2/bin/mpiexec"
CMD=( "$MPIEXEC" -bootstrap ssh -f "$TEMP_HOSTFILE" -n "$NPROCS"
      -genvlist PATH,LD_LIBRARY_PATH
      -env DISPLAY "" )

if [[ -n "$IFACE" ]]; then
    CMD+=( -iface "$IFACE" )
fi

# Ejecutar con la imagen como argumento
"${CMD[@]}" "$EXECUTABLE" "$IMAGE_FILE"
EXIT_CODE=$?

# Limpieza
rm -f "$TEMP_HOSTFILE"

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  ✓ PROCESAMIENTO COMPLETADO EXITOSAMENTE${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo "Archivos generados:"
    echo "  • result.png - Imagen procesada con filtro Sobel"
    echo "  • result_histogram.png - Histograma en formato PNG"
    echo "  • result_histogram.cvc - Histograma en formato CVC"
    echo ""
    echo "Ubicación: ~/Documents/Proyecto2-SO/MainSystem/Master/"
    echo ""
else
    echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${RED}  ✗ Error en la ejecución (código: $EXIT_CODE)${NC}"
    echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
    echo ""
fi

exit $EXIT_CODE