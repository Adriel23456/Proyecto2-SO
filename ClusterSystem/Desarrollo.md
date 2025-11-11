# 🧠 Instalación y Configuración de un Clúster Beowulf con OpenMPI

> **Objetivo:** Configurar un entorno de cómputo distribuido tipo *Beowulf* utilizando **OpenMPI** y **SSH** en sistemas basados en Linux (Ubuntu / Raspberry Pi OS).

---

## ⚙️ Requisitos previos

* Todos los nodos (master y slaves) deben estar en la **misma red local (LAN)**.
* Cada nodo debe tener instalado **Linux Ubuntu** o **Raspberry Pi OS (Debian 12+)**.
* Todos los nodos deben poder **hacer ping** entre sí.
* Se recomienda asignar **IPs fijas** o configurar las IPs manualmente en `/etc/hosts`.

---

## 🧩 Paso 1. Instalar OpenMPI en todos los nodos

Ejecuta lo siguiente en **cada nodo (Master y Slaves):**

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install openmpi-bin openmpi-common libopenmpi-dev -y
```

✅ **Verifica la instalación:**

```bash
mpirun --version
```

Debería mostrar la versión instalada de OpenMPI.

---

## 🔐 Paso 2. Instalar y habilitar SSH

### 🔸 En los nodos *Slave* (servidores controlados)

Instala el servidor SSH:

```bash
sudo apt install openssh-server -y
```

Habilita y verifica el servicio:

```bash
sudo systemctl enable ssh
sudo systemctl start ssh
sudo systemctl status ssh
```

Como extra, asegurarse de tener el mismo nombre de usuario tal que asi:

```bash
sudo adduser so-proy2
sudo usermod -aG sudo so-proy2
su - so-proy2
```

---

### 🔸 En el nodo *Master* (controlador)

Instala el cliente SSH:

```bash
sudo apt install openssh-client -y
```

---

## 🌐 Paso 3. Obtener las direcciones IP de cada nodo

Para ver la IP local de cada máquina:

```bash
hostname -I
```

Si el comando no existe, instala las herramientas de red:

```bash
sudo apt install net-tools -y
```

Luego edita el archivo `/etc/hosts` en **todos los nodos** para mapear las direcciones IP:

```bash
sudo nano /etc/hosts
```

Ejemplo de configuración:

```
192.168.18.242  master
192.168.18.10   slave1
192.168.18.241  slave2
```

Guarda con **Ctrl+O** y cierra con **Ctrl+X**.

> ⚠️ Es importante que los nombres aquí definidos coincidan con los que usaremos en el archivo de hosts MPI.

---

## 🔑 Paso 4. Generar y distribuir claves SSH (para conexión sin contraseña)

Desde el **nodo Master**, genera una clave RSA sin contraseña:

```bash
ssh-keygen -t rsa -b 4096 -N "" -f ~/.ssh/id_rsa
```

Esto crea los archivos:

* `~/.ssh/id_rsa` → clave privada
* `~/.ssh/id_rsa.pub` → clave pública

Luego, copia la clave pública a los nodos *Slave* (reemplaza `usuario` por tu nombre real):

```bash
scp ~/.ssh/id_rsa.pub usuario@192.168.18.10:~/.ssh/authorized_keys
scp ~/.ssh/id_rsa.pub usuario@192.168.18.241:~/.ssh/authorized_keys
```

En **cada nodo Slave**, ajusta los permisos:

```bash
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
```

✅ **Prueba la conexión:**

Desde el Master:

```bash
ssh usuario@192.168.18.10
ssh usuario@192.168.18.241
```

Si entras sin que te pida contraseña, la conexión SSH está correctamente configurada.

---

## 🧾 Paso 5. Crear el archivo de configuración de nodos MPI

Crea el archivo `~/.mpi_hostfile` en el **nodo Master**:

```bash
nano ~/.mpi_hostfile
```

Ejemplo de contenido:

```
localhost slots=2
slave1 slots=2
slave2 slots=2
```

Donde:

* `localhost` → el propio nodo Master.
* `slots` → número de procesos o núcleos que ese nodo puede usar.
* `slave1`, `slave2` → nombres definidos en `/etc/hosts`.

Guarda con **Ctrl+O** y cierra con **Ctrl+X**.

---

## ⚙️ Paso 6. Preparar el código a ejecutar

El programa a ejecutar (por ejemplo `ejemplo.c`) debe estar en **la misma ruta en todos los nodos**.

Compila el programa en cada máquina:

```bash
cd Documents/Proyecto2-SO/ClusterSystem
mpicc ejemplo.c -o ejemplo
```

Esto generará un binario `./ejemplo` listo para ejecutar en paralelo.

---

## 🚀 Paso 7. Ejecutar el programa desde el nodo Master

Desde el Master:

```bash
mpirun -np 4 --hostfile ~/.mpi_hostfile ./ejemplo
```

Explicación:

* `-np 4` → número total de procesos a ejecutar.
* `--hostfile ~/.mpi_hostfile` → archivo con los nodos del clúster.
* `./ejemplo` → programa compilado a ejecutar.

---

## ✅ Verificación de funcionamiento

Si la configuración fue correcta, deberías ver que los procesos se distribuyen entre el Master y los Slaves.
Puedes comprobarlo con un programa de prueba como este:

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

Compílalo y ejecútalo en el clúster.
Deberías ver múltiples líneas con diferentes `world_rank`, indicando que el cómputo está distribuido.

---

## 🧰 Consejos finales

* Asegúrate de que todos los nodos tengan **el mismo usuario** y **nombre de carpeta de trabajo**.
* Revisa que las rutas en `/etc/hosts` sean idénticas en todos los equipos.
* Puedes sincronizar carpetas con `rsync` o compartirlas por NFS si quieres mantener el mismo código centralizado.
* Si hay errores de SSH, ejecuta con `mpirun -v` para modo detallado.