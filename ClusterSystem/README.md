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
192.168.18.10   slave1 dominio1
192.168.18.241  slave2 dominio2
192.168.18.90   slave3 dominio3
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

### En Raspberry Pi (ARM7 32-bit - Master) (Nota: Se tendran que corregir manualmente archivos por retrocompatibilidad en el 'make')
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
192.168.18.242 slots=2

# Slaves x86_64
192.168.18.10  slots=2
192.168.18.241 slots=2
192.168.18.90  slots=2
```

---

## 5) Preparar el programa de prueba

### Crear directorio de trabajo en TODOS los nodos:
```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
```

### Crear y compilar el programa en TODOS los nodos:
Se tiene en el repositorio ya descargado!

### Compilar en TODOS los nodos:
```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
/opt/openmpi-4.1.6/bin/mpicc -o ejemplo ejemplo.c
```

---

## 6) Script run_mpi_safe.sh mejorado

### Crear el script en el MASTER:
Se tiene en el repositorio ya descargado!

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
/opt/openmpi-4.1.6/bin/mpirun     -np 4     --hostfile ~/.mpi_hostfile     --map-by node     --bind-to core     --report-bindings     -x PATH     -x LD_LIBRARY_PATH     ./ejemplo
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

## 📌 Notas importantes

1. **Heterogeneidad**: OpenMPI 4.1.6 maneja automáticamente las diferencias de arquitectura si se instalo con la retrocompatibilidad en mente
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