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
FINAL_HOSTFILE="/tmp/.mpi_hostfile_final"
TIMEOUT=3
IFACE=""            # Puede ser una IP o nombre de interfaz. Si está vacío, se autodetecta.
IMAGE_FILE=""

# Si pones DEBUG_KEEP_HOSTFILES=1, no borraremos los archivos temporales.
DEBUG_KEEP_HOSTFILES="${DEBUG_KEEP_HOSTFILES:-0}"

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
    echo "Uso: $0 <imagen.png> [-t TIMEOUT] [-i IFACE|IP]"
    echo ""
    echo "Parámetros:"
    echo "  <imagen.png>  - Ruta de la imagen a procesar (requerido)"
    echo "  -t TIMEOUT    - Timeout para verificar slaves (default: 3)"
    echo "  -i IFACE|IP   - Interfaz (eth0/wlan0/...) o IP local a usar (opcional)"
    echo ""
    echo "Ejemplos:"
    echo "  $0 image.png"
    echo "  $0 ~/Pictures/photo.png -t 5"
    echo "  $0 image.png -i 192.168.18.242"
    echo "  $0 image.png -i wlan0"
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

# Verificar ejecutable local
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

# ===== Verificar disponibilidad de slaves remotos =====
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}  Verificando disponibilidad de slaves...${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo ""

: > "$TEMP_HOSTFILE"
available_hosts=0
available_slots=0
configured_hosts=0

# Limpiamos final por si quedó de una corrida anterior
rm -f "$FINAL_HOSTFILE"

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
        # Verificar que el ejecutable exista en el remote (mismo path)
        if ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "[ -x '$EXECUTABLE' ]" < /dev/null 2>/dev/null; then
            echo -e "${GREEN}DISPONIBLE${NC}"
            echo "${host}:${ppn}" >> "$TEMP_HOSTFILE"
            available_hosts=$((available_hosts+1))
            available_slots=$((available_slots+ppn))
        else
            echo -e "${RED}EJECUTABLE NO ENCONTRADO${NC}"
        fi
    else
        echo -e "${RED}NO DISPONIBLE${NC}"
    fi
done 3< "$ORIGINAL_HOSTFILE"

echo ""
echo -e "${YELLOW}Resumen (remotos):${NC}"
echo "  Hosts configurados : $configured_hosts"
echo "  Hosts disponibles   : $available_hosts"
echo "  Slots disponibles   : $available_slots"
echo ""

if [ "$available_hosts" -eq 0 ]; then
    echo -e "${RED}ERROR: No hay slaves disponibles. Abortando.${NC}"
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# ===== Resolver INTERFAZ y IP del master (para anunciar) =====
resolve_iface_and_ip() {
    local hint="$1"
    local first_remote="$2"

    local ifname=""
    local ipaddr=""

    # Si hint es nombre de interfaz válido, úsalo
    if [[ -n "$hint" ]] && ip -o -4 addr show "$hint" >/dev/null 2>&1; then
        ifname="$hint"
        ipaddr="$(ip -o -4 addr show "$ifname" | awk '{print $4}' | cut -d/ -f1 | head -n1)"
        echo "$ifname;$ipaddr"
        return 0
    fi

    # Si hint parece IP, busca interfaz que tenga esa IP
    if [[ "$hint" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
        ifname="$(ip -o -4 addr | awk -v ip="$hint" '$4 ~ ip"/"{print $2; exit}')"
        if [[ -n "$ifname" ]]; then
            ipaddr="$hint"
            echo "$ifname;$ipaddr"
            return 0
        fi
    fi

    # Autodetectar interfaz hacia el primer host remoto
    if [[ -n "$first_remote" ]]; then
        ifname="$(ip route get "$first_remote" 2>/dev/null | awk '/ dev / {for(i=1;i<=NF;i++) if($i=="dev"){print $(i+1); exit}}')"
        ipaddr="$(ip route get "$first_remote" 2>/dev/null | awk '/ src / {for(i=1;i<=NF;i++) if($i=="src"){print $(i+1); exit}}')"
        if [[ -n "$ifname" && -n "$ipaddr" ]]; then
            echo "$ifname;$ipaddr"
            return 0
        fi
    fi

    # Fallback: primera interfaz con IP privada
    ifname="$(ip -o -4 addr | awk '$4 ~ /^10\.|^172\.(1[6-9]|2[0-9]|3[0-1])\.|^192\.168\./ {print $2; exit}')"
    ipaddr="$(ip -o -4 addr show "$ifname" 2>/dev/null | awk '{print $4}' | cut -d/ -f1 | head -n1)"

    # Último fallback: loopback (no ideal, pero evita vacío)
    if [[ -z "$ifname" || -z "$ipaddr" ]]; then
        ifname="lo"
        ipaddr="127.0.0.1"
    fi

    echo "$ifname;$ipaddr"
    return 0
}

first_remote_host="$(head -n1 "$TEMP_HOSTFILE" | cut -d: -f1)"
IFS=';' read -r MASTER_IFACE_NAME MASTER_IP <<< "$(resolve_iface_and_ip "$IFACE" "$first_remote_host")"

echo -e "${YELLOW}Master IFACE:${NC} $MASTER_IFACE_NAME"
echo -e "${YELLOW}Master IP anunciada:${NC} $MASTER_IP"
echo ""

# ===== Construir hostfile final: PRIMERA LÍNEA = IP del master =====
{
    echo "${MASTER_IP}:1"
    cat "$TEMP_HOSTFILE"
} > "$FINAL_HOSTFILE"

total_slots=$((available_slots + 1))   # +1 por el master
echo -e "${YELLOW}Asignación final:${NC}"
echo "  Master (rank 0)     : ${MASTER_IP} (1 slot reservado)"
echo "  Slaves (remotos)    : $available_slots slot(s)"
echo "  Slots totales       : $total_slots"
echo ""
echo -e "${YELLOW}Hostfile efectivo (orden de mapeo de ranks):${NC}"
nl -ba "$FINAL_HOSTFILE" | sed 's/^/  /'
echo ""

# ===== Ejecutar MPI =====
MPIEXEC="/usr/local/mpich-4.3.2/bin/mpiexec"

# Muy importante:
# - SOLO seteamos HYDRA_IFACE localmente (para el PMI del master).
# - NO exportamos MPICH_INTERFACE_HOSTNAME a los slaves (evita conflicto de IP).
export HYDRA_IFACE="$MASTER_IFACE_NAME"

CMD=( "$MPIEXEC"
      -bootstrap ssh
      -f "$FINAL_HOSTFILE"
      -n "$total_slots"
      -genvlist PATH,LD_LIBRARY_PATH
      -env DISPLAY ""
      -print-all-exitcodes )

echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  Ejecutando procesamiento distribuido...${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo ""
echo "  Procesos totales: $total_slots (1 master local + $available_slots slaves)"
echo "  Imagen: $IMAGE_FILE"
echo ""

# [Opcional de diagnóstico rápido] Descomenta para validar el hostfile:
# "${CMD[@]}" /bin/hostname || { echo -e "${RED}Fallo precheck hostname${NC}"; exit 1; }

# Ejecutar con la imagen como argumento
"${CMD[@]}" "$EXECUTABLE" "$IMAGE_FILE"
EXIT_CODE=$?

# Limpieza
if [ "$DEBUG_KEEP_HOSTFILES" -ne 1 ]; then
    rm -f "$TEMP_HOSTFILE" "$FINAL_HOSTFILE"
else
    echo -e "${YELLOW}[DEBUG] Conservando hostfiles:${NC} $TEMP_HOSTFILE  $FINAL_HOSTFILE"
fi

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
