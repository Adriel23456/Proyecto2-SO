# 🧠 Clúster Beowulf **heterogéneo** con **MPICH (Hydra)** — Guía limpia y replicable

> **Objetivo:** Montar un clúster Beowulf (master + slaves) mezclando **Raspberry Pi 32-bit** y **PCs x86_64**, usando **MPICH 4.3.2 (Hydra)** sobre **SSH**.
> **Resultado esperado:** `mpiexec -n 4 ./ejemplo` imprime 4 líneas “Hola desde el proceso …”.

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

   * “raspberrypi” es el **hostname real** del Pi.
   * “master” es un **alias contextual** para el Pi.
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
   ```
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

## 6) Ejecuta el clúster

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

## 7) Verificaciones rápidas (cuando algo no corre)

```bash
# SSH sin password hacia cada host
ssh -o BatchMode=yes adriel@192.168.18.10  'true' && echo OK1 || echo FAIL1
ssh -o BatchMode=yes adriel@192.168.18.241 'true' && echo OK2 || echo FAIL2

# MPICH presente en remotos
ssh adriel@192.168.18.10  'bash -lc "which mpiexec && echo LD=$LD_LIBRARY_PATH"'
ssh adriel@192.168.18.241 'bash -lc "which mpiexec && echo LD=$LD_LIBRARY_PATH"'

# Binario en misma ruta
ssh adriel@192.168.18.10  'bash -lc "ls -l ./ejemplo"'
ssh adriel@192.168.18.241 'bash -lc "ls -l ./ejemplo"'
```

---

## 8) Programa de prueba mínimo

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
/usr/local/mpich-4.3.2/bin/mpicc ejemplo.c -o ejemplo
```

---

**Con esto tienes el `.md` correcto, sin pasos innecesarios, usando `nano`, obteniendo IPs, conectando todo por SSH, instalando Hydra (MPICH 4.3.2) con heterogeneidad y ejecutando el clúster.**