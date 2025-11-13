# 🚀 Clúster Beowulf **Heterogéneo** con **OpenMPI** — ARM7 + x86_64

> **Arquitectura:** Master en Raspberry Pi OS (ARM7 32-bit) + Slaves en Ubuntu (x86_64)  
> **MPI:** OpenMPI 4.1.6 con soporte heterogéneo completo  
> **Objetivo:** Ejecutar procesos MPI distribuidos entre arquitecturas diferentes

---

## 📋 Pre-requisitos

- Raspberry Pi con Raspberry Pi OS (32-bit ARM7)
- PCs/VMs con Ubuntu 22.04/24.04 (x86_64)
- Conexión de red entre todos los nodos
- Usuario común en todos los nodos (ej: `adriel`)

---

## 0) Configuración de red y nombres

### En TODOS los nodos:

1. **Crear usuario común** (si no existe):
```bash
sudo adduser adriel
sudo usermod -aG sudo adriel
su - adriel
```

2. **Obtener IPs de cada nodo**:
```bash
hostname -I | awk '{print $1}'
```

3. **Configurar `/etc/hosts`** (ajusta con tus IPs):
```bash
sudo nano /etc/hosts
```

Contenido ejemplo:
```
192.168.18.242  raspberrypi master
192.168.18.10   slave1
192.168.18.241  slave2
192.168.18.90   slave3
```

4. **Verificar conectividad**:
```bash
ping -c 2 master
ping -c 2 slave1
```

---

## 1) Instalación de SSH

### En TODOS los nodos:
```bash
sudo apt update
sudo apt install -y openssh-server openssh-client
sudo systemctl enable --now ssh
sudo systemctl status ssh
```

---

## 2) Instalación de OpenMPI 4.1.6 con soporte heterogéneo

### ⚠️ IMPORTANTE: Usar exactamente la misma versión y prefijo en TODOS los nodos

### En Raspberry Pi (ARM7 32-bit - Master)
```bash
# Limpiar cualquier instalación previa de OpenMPI
sudo rm -rf /opt/openmpi* /usr/local/openmpi*
sed -i '/openmpi/Id' ~/.bashrc
source ~/.bashrc

# Instalar dependencias
sudo apt update
sudo apt install -y build-essential gfortran libhwloc-dev libevent-dev

# Descargar OpenMPI 4.1.6
cd ~
wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.6.tar.gz
tar xzf openmpi-4.1.6.tar.gz
cd openmpi-4.1.6

# Configurar para ARM7 32-bit con soporte heterogéneo
./configure --prefix=/opt/openmpi-4.1.6 \
            --enable-heterogeneous \
            --enable-mpi-fortran=no \
            --enable-mpirun-prefix-by-default \
            --with-hwloc=internal \
            --with-libevent=internal \
            --disable-mpi-fortran \
            --enable-shared \
            --enable-static \
            CC=gcc CXX=g++ \
            CFLAGS="-O2 -march=armv7-a -mfpu=vfp -mfloat-abi=hard" \
            CXXFLAGS="-O2 -march=armv7-a -mfpu=vfp -mfloat-abi=hard"

# Compilar e instalar
make -j$(nproc)
sudo make install

# Configurar variables de entorno
echo 'export PATH=/opt/openmpi-4.1.6/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/opt/openmpi-4.1.6/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# Registrar bibliotecas
echo '/opt/openmpi-4.1.6/lib' | sudo tee /etc/ld.so.conf.d/openmpi.conf
sudo ldconfig

# Verificar instalación
which mpicc
mpirun --version
ompi_info | grep -i heterogeneous
```

### En Slaves x86_64 (Ubuntu 64-bit)
```bash
# Limpiar instalaciones previas
sudo rm -rf /opt/openmpi* /usr/local/openmpi*
sed -i '/openmpi/Id' ~/.bashrc
source ~/.bashrc

# Instalar dependencias
sudo apt update
sudo apt install -y build-essential gfortran libhwloc-dev libevent-dev

# Descargar OpenMPI 4.1.6
cd ~
wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.6.tar.gz
tar xzf openmpi-4.1.6.tar.gz
cd openmpi-4.1.6

# Configurar para x86_64 con soporte heterogéneo
./configure --prefix=/opt/openmpi-4.1.6 \
            --enable-heterogeneous \
            --enable-mpi-fortran=no \
            --enable-mpirun-prefix-by-default \
            --with-hwloc=internal \
            --with-libevent=internal \
            --disable-mpi-fortran \
            --enable-shared \
            --enable-static \
            CC=gcc CXX=g++ \
            CFLAGS="-O2" \
            CXXFLAGS="-O2"

# Compilar e instalar
make -j$(nproc)
sudo make install

# Configurar variables de entorno
echo 'export PATH=/opt/openmpi-4.1.6/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/opt/openmpi-4.1.6/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# Registrar bibliotecas
echo '/opt/openmpi-4.1.6/lib' | sudo tee /etc/ld.so.conf.d/openmpi.conf
sudo ldconfig

# Verificar instalación
which mpicc
mpirun --version
ompi_info | grep -i heterogeneous
```

---

## 3) Configuración SSH sin contraseña

### Desde el MASTER (Raspberry Pi):
```bash
# Generar llave SSH
[ -f ~/.ssh/id_ed25519 ] || ssh-keygen -t ed25519 -N "" -f ~/.ssh/id_ed25519

# Copiar llave a todos los nodos (incluido el master mismo)
ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@localhost
ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@slave1
ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@slave2
ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@slave3

# Verificar permisos
ssh adriel@localhost 'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
ssh adriel@slave1 'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
ssh adriel@slave2 'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
ssh adriel@slave3 'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'

# Probar conexión sin contraseña
ssh -o BatchMode=yes adriel@localhost 'echo OK_MASTER'
ssh -o BatchMode=yes adriel@slave1 'echo OK_SLAVE1'
ssh -o BatchMode=yes adriel@slave2 'echo OK_SLAVE2'
ssh -o BatchMode=yes adriel@slave3 'echo OK_SLAVE3'
```

---

## 4) Crear archivo de hosts para OpenMPI

### En el MASTER:
```bash
nano ~/.mpi_hostfile
```

Contenido (ajusta IPs y slots según tu configuración):
```
# Master (Raspberry Pi ARM7)
192.168.18.242 slots=2 max_slots=4

# Slaves x86_64
192.168.18.10  slots=2 max_slots=4
192.168.18.241 slots=2 max_slots=4
192.168.18.90  slots=2 max_slots=4
```

---

## 5) Preparar el programa de prueba

### Crear directorio de trabajo en TODOS los nodos:
```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
```

### Crear y compilar el programa en TODOS los nodos:

Guarda esto como `ejemplo.c`:
```c
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int world_size, world_rank, name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    char hostname[256];
    
    // Inicializar MPI
    MPI_Init(&argc, &argv);
    
    // Obtener información del proceso
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Get_processor_name(processor_name, &name_len);
    
    // Obtener hostname del sistema
    gethostname(hostname, sizeof(hostname));
    
    // Detectar arquitectura
    #ifdef __arm__
        const char* arch = "ARM";
    #elif __x86_64__
        const char* arch = "x86_64";
    #elif __i386__
        const char* arch = "x86_32";
    #else
        const char* arch = "Unknown";
    #endif
    
    // Imprimir mensaje desde proceso externo
    printf("Hola desde proceso externo %d de %d total | Host: %s | Arch: %s | PID: %d\n", 
           world_rank, world_size, processor_name, arch, getpid());
    
    // Sincronizar todos los procesos
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Solo el proceso 0 imprime resumen
    if (world_rank == 0) {
        printf("\n=== Ejecución MPI Heterogénea Completada ===\n");
        printf("Total de procesos: %d\n", world_size);
    }
    
    // Finalizar MPI
    MPI_Finalize();
    return 0;
}
```

### Compilar en TODOS los nodos:
```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
/opt/openmpi-4.1.6/bin/mpicc -o ejemplo ejemplo.c
```

---

## 6) Script run_mpi_safe.sh mejorado

### Crear el script en el MASTER:
```bash
nano ~/Documents/Proyecto2-SO/ClusterSystem/run_mpi_safe.sh
```

Contenido:
```bash
#!/bin/bash
# Script: run_mpi_safe.sh
# Ejecuta MPI solo con nodos disponibles en clúster heterogéneo

# ===== Configuración =====
EXECUTABLE="./ejemplo"
HOSTFILE="${HOME}/.mpi_hostfile"
TEMP_HOSTFILE="/tmp/mpi_hostfile_available_$$"
TIMEOUT=3
MPI_PATH="/opt/openmpi-4.1.6"
DEFAULT_SLOTS=2

# ===== Colores para output =====
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

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
            echo "  -n, --nprocs N         Forzar número de procesos"
            echo "  -h, --help             Mostrar esta ayuda"
            exit 0
            ;;
        *)
            echo "Opción desconocida: $1"
            exit 1
            ;;
    esac
done

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

# ===== Verificar nodos =====
echo -e "${YELLOW}▶ Escaneando clúster...${NC}\n"

: > "$TEMP_HOSTFILE"
available_nodes=0
total_slots=0
configured_nodes=0

# Leer hostfile y verificar cada nodo
while IFS= read -r line; do
    # Saltar comentarios y líneas vacías
    [[ "$line" =~ ^#.*$ ]] || [ -z "$line" ] && continue
    
    # Extraer host y slots
    host=$(echo "$line" | awk '{print $1}')
    slots=$(echo "$line" | grep -o 'slots=[0-9]*' | cut -d= -f2)
    [ -z "$slots" ] && slots=$DEFAULT_SLOTS
    
    configured_nodes=$((configured_nodes + 1))
    
    printf "  Verificando %-20s " "$host"
    
    if check_node "$host" "$TIMEOUT"; then
        arch=$(get_arch "$host")
        echo -e "${GREEN}✓${NC} ONLINE [${arch}] (slots=$slots)"
        echo "$host slots=$slots" >> "$TEMP_HOSTFILE"
        available_nodes=$((available_nodes + 1))
        total_slots=$((total_slots + slots))
    else
        echo -e "${RED}✗${NC} OFFLINE"
    fi
done < "$HOSTFILE"

# ===== Resumen de disponibilidad =====
echo -e "\n${YELLOW}▶ Resumen del clúster:${NC}"
echo "  • Nodos configurados: $configured_nodes"
echo "  • Nodos disponibles:  $available_nodes"
echo "  • Slots totales:      $total_slots"

if [ "$available_nodes" -eq 0 ]; then
    echo -e "\n${RED}✗ ERROR: No hay nodos disponibles${NC}"
    rm -f "$TEMP_HOSTFILE"
    exit 1
fi

# ===== Determinar número de procesos =====
if [ -n "$FORCE_NPROCS" ]; then
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

# Construir comando MPI con opciones para heterogeneidad
$MPI_PATH/bin/mpirun \
    --hostfile "$TEMP_HOSTFILE" \
    --np "$NPROCS" \
    --hetero \
    --map-by node \
    --bind-to core \
    --report-bindings \
    -x PATH \
    -x LD_LIBRARY_PATH \
    "$EXECUTABLE"

EXIT_CODE=$?

echo -e "\n${YELLOW}════════════════════════════════════════════════${NC}"

# ===== Limpieza =====
rm -f "$TEMP_HOSTFILE"

# ===== Resultado final =====
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "\n${GREEN}✓ Ejecución completada exitosamente${NC}\n"
else
    echo -e "\n${RED}✗ Error en la ejecución (código: $EXIT_CODE)${NC}\n"
fi

exit $EXIT_CODE
```

### Hacer ejecutable y copiar globalmente:
```bash
chmod +x ~/Documents/Proyecto2-SO/ClusterSystem/run_mpi_safe.sh
sudo cp ~/Documents/Proyecto2-SO/ClusterSystem/run_mpi_safe.sh /usr/local/bin/mpirun-safe
sudo chmod +x /usr/local/bin/mpirun-safe
```

---

## 7) Ejecución del clúster

### Método 1: Con script seguro (RECOMENDADO)
```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
mpirun-safe
```

### Método 2: Ejecución directa
```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
/opt/openmpi-4.1.6/bin/mpirun \
    --hostfile ~/.mpi_hostfile \
    --np 4 \
    --hetero \
    --map-by node \
    ./ejemplo
```

---

## 8) Verificación y troubleshooting

### Verificar conectividad SSH:
```bash
for host in localhost slave1 slave2 slave3; do
    echo -n "Testing $host: "
    ssh -o BatchMode=yes $host 'echo OK' 2>/dev/null && echo "✓" || echo "✗"
done
```

### Verificar OpenMPI en todos los nodos:
```bash
for host in localhost slave1 slave2 slave3; do
    echo "=== $host ==="
    ssh $host '/opt/openmpi-4.1.6/bin/ompi_info | grep -E "heterogeneous|Open MPI:"'
done
```

### Verificar que el ejecutable existe en todos los nodos:
```bash
for host in localhost slave1 slave2 slave3; do
    ssh $host "ls -l ~/Documents/Proyecto2-SO/ClusterSystem/ejemplo" 2>/dev/null || echo "$host: No encontrado"
done
```

### Test simple local:
```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
/opt/openmpi-4.1.6/bin/mpirun -np 2 ./ejemplo
```

---

## 🎯 Resultado esperado

Al ejecutar `mpirun-safe`, deberías ver algo así:
```
╔══════════════════════════════════════════════╗
║    Sistema MPI Heterogéneo OpenMPI 4.1.6    ║
╚══════════════════════════════════════════════╝

▶ Escaneando clúster...

  Verificando 192.168.18.242      ✓ ONLINE [armv7l] (slots=2)
  Verificando 192.168.18.10       ✓ ONLINE [x86_64] (slots=2)
  Verificando 192.168.18.241      ✓ ONLINE [x86_64] (slots=2)
  Verificando 192.168.18.90       ✗ OFFLINE

▶ Resumen del clúster:
  • Nodos configurados: 4
  • Nodos disponibles:  3
  • Slots totales:      6

▶ Ejecutando MPI con 6 procesos en 3 nodos

════════════════════════════════════════════════

Hola desde proceso externo 0 de 6 total | Host: raspberrypi | Arch: ARM | PID: 12345
Hola desde proceso externo 1 de 6 total | Host: raspberrypi | Arch: ARM | PID: 12346
Hola desde proceso externo 2 de 6 total | Host: slave1 | Arch: x86_64 | PID: 23456
Hola desde proceso externo 3 de 6 total | Host: slave1 | Arch: x86_64 | PID: 23457
Hola desde proceso externo 4 de 6 total | Host: slave2 | Arch: x86_64 | PID: 34567
Hola desde proceso externo 5 de 6 total | Host: slave2 | Arch: x86_64 | PID: 34568

=== Ejecución MPI Heterogénea Completada ===
Total de procesos: 6

════════════════════════════════════════════════

✓ Ejecución completada exitosamente
```

---

## 📌 Notas importantes

1. **Heterogeneidad**: OpenMPI 4.1.6 maneja automáticamente las diferencias de arquitectura con `--hetero`
2. **Sincronización**: Todos los nodos deben tener el ejecutable en la misma ruta
3. **Bibliotecas**: Las rutas de OpenMPI deben ser idénticas en todos los nodos
4. **SSH**: La autenticación sin contraseña es obligatoria
5. **Firewall**: Asegúrate de que los puertos MPI no estén bloqueados

---

## 🛠️ Solución de problemas comunes

### Error: "cannot open shared object file"
```bash
# En todos los nodos:
sudo ldconfig
source ~/.bashrc
```

### Error: "Permission denied (publickey)"
```bash
# Desde el master:
ssh-copy-id adriel@nodo_problemático
```

### Error: "heterogeneous run attempted"
```bash
# Verificar que --hetero esté habilitado:
ompi_info | grep heterogeneous
# Debe mostrar: "Heterogeneous support: yes"
```

---

¡Con esta configuración tendrás un clúster Beowulf heterogéneo totalmente funcional! 🚀