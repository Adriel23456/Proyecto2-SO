#!/bin/bash

# Ruta del ejecutable MPI en todos los nodos
APP_DIR="$HOME/Documents/Proyecto2-SO/ClusterSystem"
EXECUTABLE="$APP_DIR/ejemplo"

# Ruta de mpirun y hostfile
MPIRUN="/opt/openmpi-4.1.6/bin/mpirun"
HOSTFILE="$HOME/.mpi_hostfile"

# Hosts a considerar (ajusta nombres si quieres)
HOSTS=(localhost slave1 slave2 slave3)

echo "==========================================="
echo "  run_mpi_safe.sh - Lanzador simple MPI"
echo "==========================================="
echo
echo "Directorio de la app:   $APP_DIR"
echo "Ejecutable:             $EXECUTABLE"
echo "Hostfile:               $HOSTFILE"
echo

# Verificaciones básicas
if [ ! -x "$MPIRUN" ]; then
    echo "ERROR: No se encontró mpirun en: $MPIRUN"
    exit 1
fi

if [ ! -f "$HOSTFILE" ]; then
    echo "ERROR: No se encontró hostfile en: $HOSTFILE"
    exit 1
fi

# Contar nodos disponibles (tienen el ejecutable)
available_hosts=0

echo "▶ Verificando nodos y ejecutable:"
for host in "${HOSTS[@]}"; do
    printf "  %s: " "$host"
    if ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "ls -l '$EXECUTABLE'" >/dev/null 2>&1; then
        echo "OK"
        available_hosts=$((available_hosts + 1))
    else
        echo "No encontrado"
    fi
done

echo
echo "Nodos disponibles con '$EXECUTABLE': $available_hosts"

if [ "$available_hosts" -eq 0 ]; then
    echo "ERROR: No hay nodos con el ejecutable disponible. Abortando."
    exit 1
fi

# 2 procesos por host disponible
NPROCS=$((available_hosts * 2))

echo "Procesos totales a lanzar (-np): $NPROCS"
echo

# Ir al directorio de trabajo
cd "$APP_DIR" || {
    echo "ERROR: No se pudo hacer cd a $APP_DIR"
    exit 1
}

echo "▶ Ejecutando mpirun..."
echo "$MPIRUN -np $NPROCS --hostfile $HOSTFILE --map-by node --bind-to core --report-bindings -x PATH -x LD_LIBRARY_PATH ./ejemplo"
echo

"$MPIRUN" \
    -np "$NPROCS" \
    --hostfile "$HOSTFILE" \
    --map-by node \
    --bind-to core \
    --report-bindings \
    -x PATH \
    -x LD_LIBRARY_PATH \
    ./ejemplo

EXIT_CODE=$?

echo
echo "==========================================="
echo " mpirun terminó con código: $EXIT_CODE"
echo "==========================================="

exit "$EXIT_CODE"
