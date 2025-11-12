#!/bin/bash
#==============================================================================
# Script: run_mpi_safe.sh (MASTER)
# Ejecuta el procesamiento distribuido verificando que todos los nodos
# usen la MISMA instalación de MPICH (ruta /usr/local/mpich-4.3.2)
#==============================================================================

set -euo pipefail

# ===== Ajusta aquí si tu MPICH está en otra ruta =====
MPICH_ROOT="/usr/local/mpich-4.3.2"
MPIEXEC="$MPICH_ROOT/bin/mpiexec"
MPICC="$MPICH_ROOT/bin/mpicc"
MPI_LIB="$MPICH_ROOT/lib"

# Forzamos PATH y LD_LIBRARY_PATH para los procesos remotos también
export PATH="$MPICH_ROOT/bin:$PATH"
export LD_LIBRARY_PATH="${MPI_LIB}:${LD_LIBRARY_PATH:-}"

# ===== Config =====
EXECUTABLE="$HOME/Documents/Proyecto2-SO/MainSystem/main"
ORIGINAL_HOSTFILE="${HYDRA_HOST_FILE:-$HOME/.mpi_hostfile}"
TEMP_HOSTFILE="/tmp/.mpi_hostfile_available"
FINAL_HOSTFILE="/tmp/.mpi_hostfile_final"
TIMEOUT=3
IFACE=""
IMAGE_FILE=""
DEBUG_KEEP_HOSTFILES="${DEBUG_KEEP_HOSTFILES:-0}"

# ===== UI =====
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'

show_help() {
    echo -e "${BLUE}Uso:${NC} $0 <imagen.png> [-t TIMEOUT] [-i IFACE|IP]"
    exit 1
}

[[ $# -lt 1 ]] && { echo -e "${RED}Falta la imagen${NC}"; show_help; }
IMAGE_FILE="$1"; shift
while getopts ":t:i:h" opt; do
  case "$opt" in
    t) TIMEOUT="$OPTARG" ;;
    i) IFACE="$OPTARG" ;;
    h) show_help ;;
    *) show_help ;;
  esac
done

echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}  Validando configuración...${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}\n"

[[ -f "$IMAGE_FILE" ]] || { echo -e "${RED}Imagen no existe:${NC} $IMAGE_FILE"; exit 1; }
echo -e "${GREEN}✓${NC} Imagen encontrada: $IMAGE_FILE"

[[ -f "$ORIGINAL_HOSTFILE" ]] || { echo -e "${RED}No existe hostfile:${NC} $ORIGINAL_HOSTFILE"; exit 1; }
echo -e "${GREEN}✓${NC} Hostfile encontrado: $ORIGINAL_HOSTFILE"

[[ -x "$EXECUTABLE" ]] || { echo -e "${RED}Ejecutable no encontrado/ejecutable:${NC} $EXECUTABLE"; exit 1; }
echo -e "${GREEN}✓${NC} Ejecutable encontrado: $EXECUTABLE"

[[ -x "$MPIEXEC" ]] || { echo -e "${RED}mpiexec no encontrado en:${NC} $MPIEXEC"; exit 1; }
echo -e "${GREEN}✓${NC} mpiexec: $MPIEXEC"

echo

# ===== Escanear slaves =====
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}  Verificando disponibilidad de slaves...${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}\n"

: > "$TEMP_HOSTFILE"
available_hosts=0
available_slots=0
configured_hosts=0

rm -f "$FINAL_HOSTFILE" || true

while IFS= read -r raw <&3 || [ -n "$raw" ]; do
  line="$(echo "$raw" | sed -E 's/^[[:space:]]+|[[:space:]]+$//g')"
  [[ -z "$line" || "$line" =~ ^# ]] && continue
  line="$(echo "$line" | sed -E 's/[[:space:]]*#.*$//')"
  [[ -z "$line" ]] && continue

  host="$(echo "$line" | cut -d':' -f1 | xargs)"
  ppn="$(echo "$line" | awk -F: 'NF>=2{print $2; exit} NF<2{print ""}')"
  [[ -z "$ppn" || ! "$ppn" =~ ^[0-9]+$ || "$ppn" -lt 1 ]] && ppn=1

  configured_hosts=$((configured_hosts+1))
  printf "  Verificando %-18s (ppn=%-2s) ... " "$host" "$ppn"

  if ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "exit" </dev/null 2>/dev/null; then
    # 1) Existe ejecutable remoto
    if ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "[ -x '$EXECUTABLE' ]" </dev/null 2>/dev/null; then
      # 2) mpiexec remoto coincide
      if ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "command -v '$MPIEXEC' >/dev/null 2>&1" </dev/null 2>/dev/null; then
        # 3) ldd del ejecutable apunta a la MISMA libmpi (MPICH_ROOT/lib)
        LIBCHK="$(ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "ldd '$EXECUTABLE' 2>/dev/null | grep -i libmpi || true")"
        if echo "$LIBCHK" | grep -q "$MPI_LIB"; then
          echo -e "${GREEN}DISPONIBLE${NC}"
          echo "${host}:${ppn}" >> "$TEMP_HOSTFILE"
          available_hosts=$((available_hosts+1))
          available_slots=$((available_slots+ppn))
        else
          echo -e "${RED}MPI LIB MISMATCH${NC}"
          echo "      ldd remoto: ${LIBCHK:-(sin salida)}"
        fi
      else
        echo -e "${RED}MPIEXEC NO COINCIDE${NC}"
      fi
    else
      echo -e "${RED}EJECUTABLE NO ENCONTRADO${NC}"
    fi
  else
    echo -e "${RED}NO DISPONIBLE${NC}"
  fi
done 3< "$ORIGINAL_HOSTFILE"

echo -e "\n${YELLOW}Resumen (remotos):${NC}"
echo "  Hosts configurados : $configured_hosts"
echo "  Hosts disponibles   : $available_hosts"
echo "  Slots disponibles   : $available_slots"
echo

[[ "$available_hosts" -ge 1 ]] || { echo -e "${RED}No hay slaves listos con MPICH consistente.${NC}"; exit 1; }

# ===== Detectar IFACE/IP del master (solo info) =====
resolve_iface_and_ip() {
  local hint="$1"; local first_remote="$2"; local ifname=""; local ipaddr=""
  if [[ -n "$hint" ]] && ip -o -4 addr show "$hint" >/dev/null 2>&1; then
    ifname="$hint"; ipaddr="$(ip -o -4 addr show "$ifname" | awk '{print $4}' | cut -d/ -f1 | head -n1)"
    echo "$ifname;$ipaddr"; return 0
  fi
  if [[ "$hint" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
    ifname="$(ip -o -4 addr | awk -v ip="$hint" '$4 ~ ip"/"{print $2; exit}')"
    [[ -n "$ifname" ]] && { echo "$ifname;$hint"; return 0; }
  fi
  if [[ -n "$first_remote" ]]; then
    ifname="$(ip route get "$first_remote" 2>/dev/null | awk '/ dev /{for(i=1;i<=NF;i++) if($i=="dev"){print $(i+1); exit}}')"
    ipaddr="$(ip route get "$first_remote" 2>/dev/null | awk '/ src /{for(i=1;i<=NF;i++) if($i=="src"){print $(i+1); exit}}')"
    [[ -n "$ifname" && -n "$ipaddr" ]] && { echo "$ifname;$ipaddr"; return 0; }
  fi
  ifname="lo"; ipaddr="127.0.0.1"; echo "$ifname;$ipaddr"
}

first_remote_host="$(head -n1 "$TEMP_HOSTFILE" | cut -d: -f1)"
IFS=';' read -r MASTER_IFACE_NAME MASTER_IP <<< "$(resolve_iface_and_ip "$IFACE" "$first_remote_host")"
echo -e "${YELLOW}Master IFACE:${NC} $MASTER_IFACE_NAME"
echo -e "${YELLOW}Master IP anunciada:${NC} $MASTER_IP\n"

# ===== Hostfile final (master primero) =====
{
  echo "${MASTER_IP}:1"
  cat "$TEMP_HOSTFILE"
} > "$FINAL_HOSTFILE"

total_slots=$((available_slots + 1))
echo -e "${YELLOW}Asignación final:${NC}"
echo "  Master (rank 0)     : ${MASTER_IP} (1 slot)"
echo "  Slaves (remotos)    : $available_slots slot(s)"
echo "  Slots totales       : $total_slots"
echo -e "\n${YELLOW}Hostfile efectivo:${NC}"
nl -ba "$FINAL_HOSTFILE" | sed 's/^/  /'
echo

# ===== Ejecutar =====
# Propagamos PATH y LD_LIBRARY_PATH a todos los procesos para blindar la lib correcta
CMD=( "$MPIEXEC"
      -bootstrap ssh
      -f "$FINAL_HOSTFILE"
      -n "$total_slots"
      -genvlist PATH,LD_LIBRARY_PATH
      -print-all-exitcodes )

echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  Ejecutando procesamiento distribuido...${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}\n"
echo "  Procesos totales: $total_slots (1 master + $available_slots slave)"
echo "  Imagen: $IMAGE_FILE"
echo

# Precheck opcional
# "${CMD[@]}" /bin/hostname

# Ejecutar la app
set +e
"${CMD[@]}" "$EXECUTABLE" "$IMAGE_FILE"
EXIT_CODE=$?
set -e

if [[ "$DEBUG_KEEP_HOSTFILES" -ne 1 ]]; then
  rm -f "$TEMP_HOSTFILE" "$FINAL_HOSTFILE" || true
else
  echo -e "${YELLOW}[DEBUG] Conservando hostfiles:${NC} $TEMP_HOSTFILE  $FINAL_HOSTFILE"
fi

echo
if [ $EXIT_CODE -eq 0 ]; then
  echo -e "${GREEN}✓ PROCESO OK${NC}"
else
  echo -e "${RED}✗ Error en la ejecución (código: $EXIT_CODE)${NC}"
fi

exit $EXIT_CODE
