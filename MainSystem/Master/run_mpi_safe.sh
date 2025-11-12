#!/bin/bash
# run_mpi_safe.sh v3 — MPICH-only env, master+slaves, hostfile robusto

set -u -o pipefail

# ===== Config MPICH =====
MPICH_ROOT="${MPICH_ROOT:-/usr/local/mpich-4.3.2}"
MPIEXEC="$MPICH_ROOT/bin/mpiexec"
MPICH_BIN="$MPICH_ROOT/bin"
MPICH_LIB="$MPICH_ROOT/lib"

# ===== Defaults =====
EXECUTABLE="$HOME/Documents/Proyecto2-SO/MainSystem/main"
ORIGINAL_HOSTFILE="${HYDRA_HOST_FILE:-$HOME/.mpi_hostfile}"
TEMP_HOSTFILE="/tmp/.mpi_hostfile_available"
FINAL_HOSTFILE="/tmp/.mpi_hostfile_final"
TIMEOUT=3
IFACE=""
REQ_NPROCS=""

while getopts ":n:e:f:t:i:" opt; do
  case "$opt" in
    n) REQ_NPROCS="$OPTARG" ;;
    e) EXECUTABLE="$OPTARG" ;;
    f) ORIGINAL_HOSTFILE="$OPTARG" ;;
    t) TIMEOUT="$OPTARG" ;;
    i) IFACE="$OPTARG" ;;
    \?) echo "Uso: $0 [-n NPROCS] [-e EXEC] [-f HOSTFILE] [-t TIMEOUT] [-i IFACE]"; exit 1 ;;
  esac
done

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}"
echo -e "${YELLOW}  Validando configuración...${NC}"
echo -e "${YELLOW}═══════════════════════════════════════════════════════════${NC}\n"

[ -x "$MPIEXEC" ] || { echo -e "${RED}✗ mpiexec no encontrado: $MPIEXEC${NC}"; exit 1; }
[ -f "$EXECUTABLE" ] || { echo -e "${RED}✗ Ejecutable no encontrado: $EXECUTABLE${NC}"; exit 1; }
[ -f "$ORIGINAL_HOSTFILE" ] || { echo -e "${RED}✗ Hostfile no encontrado: $ORIGINAL_HOSTFILE${NC}"; exit 1; }

echo "✓ Ejecutable: $EXECUTABLE"
echo "✓ Hostfile  : $ORIGINAL_HOSTFILE"
echo "✓ mpiexec   : $MPIEXEC"
echo

IMG_ARG=""
if [[ $# -ge 1 && -f "$1" && "$1" =~ \.png$ ]]; then
  IMG_ARG="$1"
  echo "✓ Imagen: $IMG_ARG"
  echo
fi

# ===== Descubrir master =====
MASTER_HOST="$(hostname -s)"
MASTER_IP="$(ip -o -4 addr show | awk '/eth0|enp|eno|wlan0/ {sub(/\/.*/,"",$4); print $4; exit}')"
if [[ -z "$MASTER_IP" ]]; then MASTER_IP="127.0.0.1"; fi

echo -e "${YELLOW}Verificando disponibilidad de slaves...${NC}\n"

: > "$TEMP_HOSTFILE"
: > "$FINAL_HOSTFILE"

available_hosts=0
available_slots=0
configured_hosts=0

# Leer hostfile de forma robusta (sin '||' en el while)
mapfile -t lines < "$ORIGINAL_HOSTFILE"
for raw in "${lines[@]}"; do
  line="$(echo "$raw" | sed -E 's/^[[:space:]]+//; s/[[:space:]]+$//')"
  [[ -z "$line" || "$line" =~ ^# ]] && continue
  line="$(echo "$line" | sed -E 's/[[:space:]]*#.*$//')"
  [[ -z "$line" ]] && continue

  host="$(echo "$line" | cut -d':' -f1 | xargs)"
  ppn="$( echo "$line" | awk -F: 'NF>=2{print $2; exit} NF<2{print ""}' )"
  [[ -z "$ppn" ]] && ppn=1
  if ! [[ "$ppn" =~ ^[0-9]+$ ]] || [ "$ppn" -lt 1 ]; then ppn=1; fi

  configured_hosts=$((configured_hosts+1))
  printf "  Verificando %-18s (ppn=%-2s) ... " "$host" "$ppn"

  set +e
  ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "exit" < /dev/null &>/dev/null
  rc=$?
  set -e
  if [ $rc -eq 0 ]; then
    echo -e "${GREEN}DISPONIBLE${NC}"
    echo "${host}:${ppn}" >> "$TEMP_HOSTFILE"
    available_hosts=$((available_hosts+1))
    available_slots=$((available_slots+ppn))
  else
    echo -e "${RED}NO DISPONIBLE${NC}"
  fi
done

echo
echo -e "${YELLOW}Resumen (remotos):${NC}"
echo "  Hosts configurados : $configured_hosts"
echo "  Hosts disponibles   : $available_hosts"
echo "  Slots disponibles   : $available_slots"

if [ "$available_hosts" -eq 0 ]; then
  echo -e "${RED}✗ No hay hosts remotos disponibles. Abortando.${NC}"
  [[ -z "${DEBUG_KEEP_HOSTFILES:-}" ]] && rm -f "$TEMP_HOSTFILE" "$FINAL_HOSTFILE"
  exit 1
fi

# ===== Construir hostfile final (master + remotos) =====
echo "${MASTER_IP}:1" > "$FINAL_HOSTFILE"
cat "$TEMP_HOSTFILE" >> "$FINAL_HOSTFILE"

TOTAL_SLOTS=$((available_slots + 1)) # +1 para master

if [[ -n "${REQ_NPROCS:-}" ]]; then
  [[ "$REQ_NPROCS" =~ ^[0-9]+$ && "$REQ_NPROCS" -ge 1 ]] || { echo -e "${RED}✗ -n inválido${NC}"; exit 1; }
  if [ "$REQ_NPROCS" -gt "$TOTAL_SLOTS" ]; then
    echo -e "${YELLOW}Aviso:${NC} ajustando -n $REQ_NPROCS -> $TOTAL_SLOTS"
    NPROCS="$TOTAL_SLOTS"
  else
    NPROCS="$REQ_NPROCS"
  fi
else
  NPROCS="$TOTAL_SLOTS"
fi

CLEAN_PATH="$MPICH_BIN:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
CLEAN_LD="$MPICH_LIB"

echo
echo "Master host       : $MASTER_HOST ($MASTER_IP)"
echo -e "${YELLOW}Hostfile efectivo:${NC}"
awk '{printf "       %s\t%s\n", NR, $0}' "$FINAL_HOSTFILE"
echo
echo -e "${YELLOW}Ejecutando procesamiento distribuido...${NC}\n"
echo "  Procesos totales : $NPROCS"
[[ -n "$IMG_ARG" ]] && echo "  Imagen           : $IMG_ARG"
echo

CMD=( "$MPIEXEC" -bootstrap ssh -f "$FINAL_HOSTFILE" -n "$NPROCS" -l
      -genv PATH "$CLEAN_PATH" -genv LD_LIBRARY_PATH "$CLEAN_LD" -env DISPLAY "" )
[[ -n "$IFACE" ]] && CMD+=( -iface "$IFACE" )

if [[ -n "${DEBUG_SMOKE:-}" ]]; then
  echo -e "${YELLOW}[SMOKE] hostname en todos los ranks...${NC}"
  "${CMD[@]}" /bin/hostname || true
  echo
fi

if [[ -n "$IMG_ARG" ]]; then
  CMD+=( "$EXECUTABLE" "$IMG_ARG" )
else
  CMD+=( "$EXECUTABLE" )
fi

set +e
"${CMD[@]}"
EXIT_CODE=$?
set -e

[[ -z "${DEBUG_KEEP_HOSTFILES:-}" ]] && rm -f "$TEMP_HOSTFILE" "$FINAL_HOSTFILE"

echo
if [ $EXIT_CODE -eq 0 ]; then
  echo -e "${GREEN}✓ Ejecución completada exitosamente${NC}"
else
  echo -e "${RED}✗ Error en la ejecución (código: $EXIT_CODE)${NC}"
fi
exit $EXIT_CODE
