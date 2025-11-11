#!/bin/bash
# Script: run_mpi_safe.sh
# Descripción: Ejecuta MPI solo con los slaves disponibles

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Archivo de hosts original
ORIGINAL_HOSTFILE=~/.mpi_hostfile
TEMP_HOSTFILE=/tmp/.mpi_hostfile_available

# Nombre del ejecutable (puedes cambiarlo)
EXECUTABLE="./ejemplo"

# Timeout para verificar conectividad (en segundos)
TIMEOUT=3

echo -e "${YELLOW}=== Sistema MPI Robusto ===${NC}"
echo ""

# Verificar que existe el hostfile original
if [ ! -f "$ORIGINAL_HOSTFILE" ]; then
    echo -e "${RED}ERROR: No se encuentra el archivo $ORIGINAL_HOSTFILE${NC}"
    exit 1
fi

# Verificar que existe el ejecutable
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}ERROR: No se encuentra el ejecutable $EXECUTABLE${NC}"
    exit 1
fi

# Limpiar archivo temporal si existe
> "$TEMP_HOSTFILE"

echo -e "${YELLOW}Verificando disponibilidad de nodos...${NC}"
echo ""

available_count=0
total_count=0

# Leer cada línea del hostfile original
# SOLUCIÓN: Usar descriptor de archivo 3 para evitar que SSH consuma stdin
while IFS= read -r line <&3 || [ -n "$line" ]; do
    # Ignorar líneas vacías o comentarios
    [[ -z "$line" || "$line" =~ ^#.* ]] && continue
    
    # Extraer solo el hostname (en caso de que tenga formato "hostname:cores")
    host=$(echo "$line" | cut -d':' -f1 | xargs)
    
    total_count=$((total_count + 1))
    
    echo -n "Verificando $host... "
    
    # SOLUCIÓN: Redirigir stdin de ssh a /dev/null para no consumir el bucle
    ssh -o ConnectTimeout=$TIMEOUT -o BatchMode=yes -o StrictHostKeyChecking=no "$host" "exit" < /dev/null 2>/dev/null
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ DISPONIBLE${NC}"
        echo "$line" >> "$TEMP_HOSTFILE"
        available_count=$((available_count + 1))
    else
        echo -e "${RED}✗ NO DISPONIBLE${NC}"
    fi
done 3< "$ORIGINAL_HOSTFILE"

echo ""
echo -e "${YELLOW}Resumen:${NC}"
echo "  Total de nodos configurados: $total_count"
echo "  Nodos disponibles: $available_count"
echo ""

# Verificar que hay al menos un nodo disponible
if [ $available_count -eq 0 ]; then
    echo -e "${RED}ERROR: No hay slaves disponibles. No se puede ejecutar MPI.${NC}"
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# Calcular número de procesos (puedes ajustar esta lógica)
# Aquí asumo 1 proceso por nodo disponible
NUM_PROCESSES=$available_count

echo -e "${GREEN}Ejecutando MPI con $NUM_PROCESSES procesos en $available_count nodos...${NC}"
echo ""

# Ejecutar MPI con el hostfile de nodos disponibles
mpiexec -n $NUM_PROCESSES -f "$TEMP_HOSTFILE" -env DISPLAY "" "$EXECUTABLE"

EXIT_CODE=$?

# Limpiar archivo temporal
rm -f "$TEMP_HOSTFILE"

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ Ejecución completada exitosamente${NC}"
else
    echo -e "${RED}✗ Error en la ejecución (código: $EXIT_CODE)${NC}"
fi

exit $EXIT_CODE