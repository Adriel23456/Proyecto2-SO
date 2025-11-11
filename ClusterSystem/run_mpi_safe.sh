#!/bin/bash
# Script: run_mpi_safe.sh
# Ejecuta MPI sólo con los nodos disponibles respetando host:ppn

# ===== Opciones =====
EXECUTABLE="./ejemplo"
ORIGINAL_HOSTFILE="${HYDRA_HOST_FILE:-$HOME/.mpi_hostfile}"
TEMP_HOSTFILE="/tmp/.mpi_hostfile_available"
TIMEOUT=3
IFACE=""
REQ_NPROCS=""

# Args simples: -n, -e, -f, -t, -i
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

# ===== Colores =====
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

echo -e "${YELLOW}=== Sistema MPI Robusto (host:ppn) ===${NC}\n"

# ===== Validaciones básicas =====
if [ ! -f "$ORIGINAL_HOSTFILE" ]; then
  echo -e "${RED}ERROR: No se encuentra $ORIGINAL_HOSTFILE${NC}"; exit 1
fi
if [ ! -f "$EXECUTABLE" ]; then
  echo -e "${RED}ERROR: No se encuentra el ejecutable $EXECUTABLE${NC}"; exit 1
fi

# ===== Preparación =====
: > "$TEMP_HOSTFILE"
available_hosts=0
available_slots=0
configured_hosts=0

echo -e "${YELLOW}Verificando disponibilidad de nodos...${NC}\n"

# Leer con FD 3 para que ssh no consuma stdin
while IFS= read -r raw <&3 || [ -n "$raw" ]; do
  # Limpia espacios y comentarios al inicio
  line="$(echo "$raw" | sed -E 's/^[[:space:]]+//; s/[[:space:]]+$//')"
  # Descarta líneas vacías y comentarios puros
  [[ -z "$line" || "$line" =~ ^# ]] && continue
  # Si hay comentario inline, córtalo
  line="$(echo "$line" | sed -E 's/[[:space:]]*#.*$//')"
  [[ -z "$line" ]] && continue

  # host[:ppn]
  host="$(echo "$line" | cut -d':' -f1 | xargs)"
  ppn="$( echo "$line" | awk -F: 'NF>=2{print $2; exit} NF<2{print ""}' )"
  [[ -z "$ppn" ]] && ppn=1

  # Valida ppn numérico y >=1
  if ! [[ "$ppn" =~ ^[0-9]+$ ]] || [ "$ppn" -lt 1 ]; then
    echo -e "  $host: ${RED}ppn inválido \"$ppn\"; usando 1${NC}"
    ppn=1
  fi

  configured_hosts=$((configured_hosts+1))
  printf "  Verificando %-18s (ppn=%-2s) ... " "$host" "$ppn"

  # Prueba ssh sin password
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

echo
echo -e "${YELLOW}Resumen:${NC}"
echo "  Hosts configurados : $configured_hosts"
echo "  Hosts disponibles   : $available_hosts"
echo "  Slots disponibles   : $available_slots"

if [ "$available_hosts" -eq 0 ]; then
  echo -e "${RED}ERROR: No hay hosts disponibles. Abortando.${NC}"
  rm -f "$TEMP_HOSTFILE"
  exit 1
fi

# ===== Determina NPROCS =====
if [[ -n "$REQ_NPROCS" ]]; then
  if ! [[ "$REQ_NPROCS" =~ ^[0-9]+$ ]] || [ "$REQ_NPROCS" -lt 1 ]; then
    echo -e "${RED}ERROR: valor de -n inválido: $REQ_NPROCS${NC}"
    rm -f "$TEMP_HOSTFILE"; exit 1
  fi
  NPROCS="$REQ_NPROCS"
  if [ "$NPROCS" -gt "$available_slots" ]; then
    echo -e "${YELLOW}Aviso:${NC} solicitaste -n $NPROCS pero sólo hay $available_slots slots; ajustando a $available_slots."
    NPROCS="$available_slots"
  fi
else
  # Por defecto usa todos los slots disponibles
  NPROCS="$available_slots"
fi

echo -e "\n${GREEN}Ejecutando:${NC} mpiexec -n $NPROCS (slots=$available_slots) en $available_hosts host(s)\n"

# ===== Construye cmd de mpiexec =====
MPIEXEC="/usr/local/mpich-4.3.2/bin/mpiexec"
CMD=( "$MPIEXEC" -bootstrap ssh -f "$TEMP_HOSTFILE" -n "$NPROCS"
      -genvlist PATH,LD_LIBRARY_PATH
      -env DISPLAY "" -print-rank-map -prepend-rank )

# iface opcional
if [[ -n "$IFACE" ]]; then
  CMD+=( -iface "$IFACE" )
fi

# Ejecuta
"${CMD[@]}" "$EXECUTABLE"
EXIT_CODE=$?

# Limpieza
rm -f "$TEMP_HOSTFILE"

echo
if [ $EXIT_CODE -eq 0 ]; then
  echo -e "${GREEN}✓ Ejecución completada exitosamente${NC}"
else
  echo -e "${RED}✗ Error en la ejecución (código: $EXIT_CODE)${NC}"
fi

exit $EXIT_CODE
