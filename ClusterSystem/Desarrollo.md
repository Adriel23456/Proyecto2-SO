# 🧠 Clúster Beowulf **heterogéneo** con **MPICH (Hydra)** — Guía limpia y replicable

> **Objetivo:** Montar un clúster Beowulf (master + slaves) mezclando **Raspberry Pi 32-bit** y **PCs x86_64**, usando **MPICH 4.3.2 (Hydra)** sobre **SSH**.
> **Resultado esperado:** `mpiexec -n 4 ./ejemplo` imprime 4 líneas "Hola desde el proceso …".

---

## 0) Nombres e IPs coherentes (sin borrar nada especial)

1. **Obtén la IP de cada nodo**:

   ```bash
   hostname -I
   ```

   > Si no tienes la herramienta en algún nodo: `sudo apt install -y net-tools` (opcional).

2. **Edita `/etc/hosts` en *todos* los nodos con `nano`**:

   ```bash
   sudo nano /etc/hosts
   ```

   **EJEMPLO (ajusta tus IPs reales):**

   ```
   192.168.18.242  raspberrypi master
   192.168.18.10   slave1
   192.168.18.241  slave2
   ```

   * "raspberrypi" es el **hostname real** del Pi.
   * "master" es un **alias contextual** para el Pi.
   * No es necesario eliminar ni comentar líneas; solo asegura que el **nombre coincida con su IP**.

3. **Verifica desde cada nodo**:

   ```bash
   getent hosts raspberrypi
   ping -c 2 raspberrypi
   ```

---

## 1) Instala e inicializa **SSH** en cada nodo

> Repite en **Pi y en cada PC** (master y slaves).

```bash
sudo apt update
sudo apt install -y openssh-server openssh-client
sudo systemctl enable --now ssh
sudo systemctl status ssh
```

* **Mismo usuario** en todos los nodos (recomendado):

  ```bash
  # si lo necesitas
  sudo adduser adriel
  sudo usermod -aG sudo adriel
  su - adriel
  ```

---

## 2) Instala **MPICH 4.3.2 (Hydra)** con soporte heterogéneo

> Repite en **todos los nodos** (Pi y PCs). Mismo `--prefix` y **mismos flags** en todos.

### Raspberry Pi (ARM 32-bit)

```bash
cd ~
[ -d mpich-4.3.2 ] || (wget https://www.mpich.org/static/downloads/4.3.2/mpich-4.3.2.tar.gz && tar xzf mpich-4.3.2.tar.gz)
cd mpich-4.3.2
make distclean 2>/dev/null || true

./configure --prefix=/usr/local/mpich-4.3.2 \
  --disable-fortran \
  --with-device=ch3:sock

make -j$(nproc)
sudo make install

echo 'export PATH=/usr/local/mpich-4.3.2/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/mpich-4.3.2/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

echo '/usr/local/mpich-4.3.2/lib' | sudo tee /etc/ld.so.conf.d/mpich-4.3.2.conf
sudo ldconfig

which mpiexec
mpiexec -n 1 bash -lc 'echo OK on $(uname -m)'
```

### Slaves x86_64 (PCs Ubuntu 64-bit)

```bash
cd ~
[ -d mpich-4.3.2 ] || (wget https://www.mpich.org/static/downloads/4.3.2/mpich-4.3.2.tar.gz && tar xzf mpich-4.3.2.tar.gz)
cd mpich-4.3.2
make distclean 2>/dev/null || true

./configure --prefix=/usr/local/mpich-4.3.2 \
  --disable-fortran \
  --with-device=ch3:sock

make -j$(nproc)
sudo make install

echo 'export PATH=/usr/local/mpich-4.3.2/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/mpich-4.3.2/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

echo '/usr/local/mpich-4.3.2/lib' | sudo tee /etc/ld.so.conf.d/mpich-4.3.2.conf
sudo ldconfig

which mpiexec
mpiexec -n 1 bash -lc 'echo OK on $(uname -m)'
```

---

## 3) Configura **SSH sin contraseña** (desde el *launcher* hacia todos)

> El *launcher* puede ser el Pi o un PC; elige uno y realiza estos pasos **solo allí**.

1. **Genera llave (si no existe)**:

   ```bash
   [ -f ~/.ssh/id_ed25519 ] || ssh-keygen -t ed25519 -N "" -f ~/.ssh/id_ed25519
   ```

2. **Instala la llave pública en cada nodo (usa tus nombres o IPs)**:

   ```bash
   ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@raspberrypi
   ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@slave1
   ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@slave2
   ```

3. **Permisos correctos remotos (por si acaso)**:

   ```bash
   ssh adriel@raspberrypi 'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
   ssh adriel@slave1     'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
   ssh adriel@slave2     'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
   ```

4. **Prueba sin password**:

   ```bash
   ssh -o BatchMode=yes adriel@raspberrypi 'true' && echo OK_PI
   ssh -o BatchMode=yes adriel@slave1     'true' && echo OK_S1
   ssh -o BatchMode=yes adriel@slave2     'true' && echo OK_S2
   ```

---

## 4) Crea el **hostfile de Hydra** (solo en el *launcher*)

1. **Edita con `nano`**:

   ```bash
   nano ~/.mpi_hostfile
   ```
2. **Contenido (formato Hydra = `host:PPN`)**:

   ```
   192.168.18.10:2
   192.168.18.241:2
   192.168.18.99:2
   192.168.18.50:2
   ```
   
   > **Nota:** Puedes incluir **todos** los slaves potenciales del clúster, incluso si no están siempre encendidos. El script `mpirun-safe` (sección 6) detectará automáticamente cuáles están disponibles.

3. **(Opcional) Exporta variable para no pasar `-f`**:

   ```bash
   echo 'export HYDRA_HOST_FILE=~/.mpi_hostfile' >> ~/.bashrc
   source ~/.bashrc
   ```
   
---

## 5) Prepara el código y compila (misma ruta en todos)

1. **Crea/asegura la ruta** en **cada nodo**:

   ```bash
   mkdir -p ~/Documents/Proyecto2-SO/ClusterSystem
   ```

2. **Coloca `ejemplo.c`** en esa ruta en **todos** los nodos.

3. **Compila con ESTE MPICH en todos**:

   ```bash
   cd ~/Documents/Proyecto2-SO/ClusterSystem
   /usr/local/mpich-4.3.2/bin/mpicc ejemplo.c -o ejemplo
   ```

---

## 6) 🚀 Script robusto: `mpirun-safe` (RECOMENDADO)

> **Problema resuelto:** Si un slave está apagado, `mpiexec` normalmente falla. Este script **detecta automáticamente** qué nodos están disponibles y ejecuta MPI solo con ellos.

### 6.1) Crea el script `run_mpi_safe.sh`

```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
nano run_mpi_safe.sh
```

**Contenido completo del script:**

```bash
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
```

### 6.2) Instala el script globalmente

```bash
# Dale permisos de ejecución
chmod +x run_mpi_safe.sh

# Copia el script a un lugar accesible globalmente
sudo cp run_mpi_safe.sh /usr/local/bin/mpirun-safe

# Asegura permisos de ejecución
sudo chmod +x /usr/local/bin/mpirun-safe
```

### 6.3) Uso del script

Desde cualquier directorio donde esté tu ejecutable MPI:

```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
mpirun-safe
```

**Salida esperada:**

```
=== Sistema MPI Robusto ===

Verificando disponibilidad de nodos...

Verificando 192.168.18.10... ✓ DISPONIBLE
Verificando 192.168.18.241... ✓ DISPONIBLE
Verificando 192.168.18.99... ✗ NO DISPONIBLE
Verificando 192.168.18.50... ✗ NO DISPONIBLE

Resumen:
  Total de nodos configurados: 4
  Nodos disponibles: 2

Ejecutando MPI con 2 procesos en 2 nodos...

Hola desde el proceso 0 de 2
Hola desde el proceso 1 de 2

✓ Ejecución completada exitosamente
```

### 6.4) Ventajas de usar `mpirun-safe`

✅ **Detecta automáticamente** qué slaves están encendidos  
✅ **Nunca crashea** si un slave está apagado  
✅ **Reporte visual claro** del estado del clúster  
✅ **No requiere editar manualmente** el hostfile  
✅ **Ejecuta con todos los nodos disponibles** automáticamente  
✅ **Portable**: Funciona desde cualquier directorio  

---

## 7) Ejecuta el clúster (método tradicional)

> **Nota:** Si instalaste `mpirun-safe` (sección 6), **úsalo siempre** en lugar de estos comandos. Los métodos a continuación funcionan pero **fallarán** si algún slave del hostfile está apagado.

* **Evita X11**:

  ```bash
  unset DISPLAY
  ```

* **Con la variable `HYDRA_HOST_FILE`**:

  ```bash
  mpiexec -n 4 -env DISPLAY "" ./ejemplo
  ```

* **O pasando el hostfile explícito**:

  ```bash
  mpiexec -f ~/.mpi_hostfile -n 4 -env DISPLAY "" ./ejemplo
  ```

* **Si necesitas fijar interfaz/IP del launcher** (ruteo estricto):

  ```bash
  mpiexec -iface 192.168.18.242 -f ~/.mpi_hostfile -n 4 -env DISPLAY "" ./ejemplo
  ```

**Salida esperada:**

```
Hola desde el proceso 0 de 4
Hola desde el proceso 1 de 4
Hola desde el proceso 2 de 4
Hola desde el proceso 3 de 4
```

---

## 8) Verificaciones rápidas (cuando algo no corre)

```bash
# SSH sin password hacia cada host
ssh -o BatchMode=yes adriel@192.168.18.10  'true' && echo OK1 || echo FAIL1
ssh -o BatchMode=yes adriel@192.168.18.241 'true' && echo OK2 || echo FAIL2

# MPICH presente en remotos
ssh adriel@192.168.18.10  'bash -lc "which mpiexec && echo LD=$LD_LIBRARY_PATH"'
ssh adriel@192.168.18.241 'bash -lc "which mpiexec && echo LD=$LD_LIBRARY_PATH"'

# Binario en misma ruta
ssh adriel@192.168.18.10  'bash -lc "ls -l ~/Documents/Proyecto2-SO/ClusterSystem/ejemplo"'
ssh adriel@192.168.18.241 'bash -lc "ls -l ~/Documents/Proyecto2-SO/ClusterSystem/ejemplo"'
```

---

## 9) Programa de prueba mínimo

```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int world_size, world_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    printf("Hola desde el proceso %d de %d\n", world_rank, world_size);
    MPI_Finalize();
    return 0;
}
```

**Compila en cada nodo**:

```bash
cd ~/Documents/Proyecto2-SO/ClusterSystem
/usr/local/mpich-4.3.2/bin/mpicc ejemplo.c -o ejemplo
```

---

## 🎯 Resumen de flujo de trabajo recomendado

1. **Configura todos los nodos** (secciones 0-5)
2. **Instala `mpirun-safe`** (sección 6)
3. **Compila tu código** en todos los nodos
4. **Ejecuta con `mpirun-safe`** desde el master
5. **¡Disfruta de un clúster resiliente!** 🚀

---

**Con esto tienes un clúster Beowulf heterogéneo completamente funcional y robusto, que tolera nodos caídos sin problemas.**








# Desde el master hacia los slaves
ping -c 2 192.168.18.10
ping -c 2 192.168.18.241
ping -c 2 raspberrypi  # Debe responder con tu IP del master

# Desde los slaves hacia el master
ssh adriel@192.168.18.10 'ping -c 2 192.168.18.242'
ssh adriel@192.168.18.10 'ping -c 2 raspberrypi'

ssh adriel@192.168.18.241 'ping -c 2 192.168.18.242'
ssh adriel@192.168.18.241 'ping -c 2 raspberrypi'