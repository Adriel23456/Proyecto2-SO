# 🧠 Clúster Beowulf **heterogéneo** con **MPICH (Hydra)** — Guía limpia y replicable

> **Objetivo:** Montar un clúster Beowulf (master + slaves) mezclando **Raspberry Pi 32-bit** y **PCs x86_64**, usando **MPICH 4.3.2 (Hydra)** sobre **SSH**.
> **Resultado esperado:** `mpiexec -n 4 ./ejemplo` imprime 4 líneas "Hola desde el proceso …".

---

## 0) Nombres e IPs coherentes (sin borrar nada especial)

* **Mismo usuario** en todos los nodos (recomendado):

```bash
# si lo necesitas
sudo adduser adriel
sudo usermod -aG sudo adriel
su - adriel
```

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
   ssh-copy-id -i ~/.ssh/id_ed25519.pub adriel@slave3
   ```

3. **Permisos correctos remotos (por si acaso)**:

   ```bash
   ssh adriel@raspberrypi 'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
   ssh adriel@slave1     'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
   ssh adriel@slave2     'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
   ssh adriel@slave3     'chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys'
   ```

4. **Prueba sin password**:

   ```bash
   ssh -o BatchMode=yes adriel@raspberrypi 'true' && echo OK_PI
   ssh -o BatchMode=yes adriel@slave1     'true' && echo OK_S1
   ssh -o BatchMode=yes adriel@slave2     'true' && echo OK_S2
   ssh -o BatchMode=yes adriel@slave3     'true' && echo OK_S3
   ```

---

## 4) Crea el **hostfile de Hydra** (solo en el *launcher*)

1. **Edita con `nano`**:

   ```bash
   nano ~/.mpi_hostfile
   ```
2. **Contenido (formato Hydra = `host:PPN`)**:

   ```                                              
   192.168.18.10:1
   #slave1

   192.168.18.241:1
   #slave2

   192.168.18.90:1
   #slave3
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
   cd ~/Documents
   git clone https://github.com/Adriel23456/Proyecto2-SO.git
   ```

2. **Compila con ESTE MPICH en todos**:

   ```bash
   cd ~/Documents/Proyecto2-SO/ClusterSystem
   /usr/local/mpich-4.3.2/bin/mpicc ejemplo.c -o ejemplo
   ```

---

## 6) 🚀 Script robusto: `mpirun-safe` (RECOMENDADO)

> **Problema resuelto:** Si un slave está apagado, `mpiexec` normalmente falla. Este script **detecta automáticamente** qué nodos están disponibles y ejecuta MPI solo con ellos.

### 6.1) Crea el script `run_mpi_safe.sh`

Ya esta creado por el repositorio de github!

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

**Compilacion en cada nodo**:

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
