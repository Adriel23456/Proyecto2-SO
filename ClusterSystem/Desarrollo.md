# 🧠 Instalación y Configuración de un Clúster MPI (Master/Slave)

> **Objetivo:** Configurar un entorno de cómputo distribuido con OpenMPI y SSH en sistemas modernos (Ubuntu / Raspberry Pi OS).

---

## 🧩 Paso 1. Instalación de OpenMPI en todos los nodos (Master y Slaves)

### 🔹 Ubuntu 25.04 / Raspberry Pi OS (Debian 12 base)

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install openmpi-bin openmpi-common libopenmpi-dev -y
```

✅ **Verificar instalación:**

```bash
mpirun --version
```

---

## 🔐 Paso 2. Instalación de OpenSSH Server (Solo en nodos *Slave*)

El servidor SSH permite que el nodo maestro controle los nodos esclavos.

```bash
sudo apt install openssh-server -y
```

✅ **Verificar que el servicio esté activo:**

```bash
sudo systemctl enable ssh
sudo systemctl start ssh
sudo systemctl status ssh
```

---

## 💻 Paso 3. Instalación de OpenSSH Client (Solo en el nodo *Master*)

El cliente SSH es necesario para que el nodo maestro pueda comunicarse con los esclavos.

```bash
sudo apt install openssh-client -y
```

---

De aqui en adelante esta raro porque lo que hacemos primero es obtener las ip de los master + slaves:
hostname -I

Asegurarse de instalar esto con:
sudo apt install net-tools

Luego obtenemos las direcciones IP y las asignamos tal que asi:
--- (TODOS)
192.168.18.242  Master
192.168.18.10   Slave1
192.168.18.241  Slave2
etc
---
Esto usando el comando de:
sudo nano /etc/hosts

LEUgo con ctrl+o y ctrl+x guardamos y salimos!


## 🔑 Paso 4. Generación de Claves SSH (En todos los nodos)

Genera una clave **sin contraseña** para permitir la conexión automática entre nodos SOLO EN EL MASTER:

```bash
ssh-keygen -t rsa -b 4096 -N "" -f ~/.ssh/id_rsa
```

> 🔸 Usa `rsa` o `ed25519`

Luego nos vamos a asegurar de que el nodo maestro este autorizado y para hacer esto, para esto el nodo maestro aplica estos comandos:
scp ~/.ssh/id_rsa.pub adriel@192.168.18.10:~/.ssh/authorized_keys
scp ~/.ssh/id_rsa.pub adriel@192.168.18.241:~/.ssh/authorized_keys




Luego, copia la clave pública del nodo maestro a cada nodo esclavo:

```bash
ssh-copy-id usuario@slave1
ssh-copy-id usuario@slave2
```

(O reemplaza `usuario` y los nombres según tu configuración.)

---

## 🗂️ Paso 5. Configuración del Archivo `/etc/hosts`

Edita este archivo en **todos los nodos** para asignar nombres legibles a las IPs de cada máquina:

```bash
sudo nano /etc/hosts
```

Agrega líneas como las siguientes:

```
192.168.1.10 master
192.168.1.11 slave1
192.168.1.12 slave2
```

> Esto permitirá conectar los nodos usando sus nombres en lugar de direcciones IP.

---

## 🔧 Paso 6. Prueba de Conectividad SSH

Desde el nodo maestro:

```bash
ssh master
ssh slave1
ssh slave2
```

Si no pide contraseña, la configuración es correcta ✅

---

## 🧪 Paso 7. Prueba de OpenMPI

Crea un archivo de prueba llamado `test_mpi.c`:

```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    printf("Hello from processor %d of %d\n", world_rank, world_size);
    MPI_Finalize();
    return 0;
}
```

Compila y ejecuta:

```bash
mpicc test_mpi.c -o test_mpi
mpirun -np 4 -host master,slave1,slave2 ./test_mpi
```

---

## ✅ Resultado Esperado

Salida similar en consola:

```
Hello from processor 0 of 4
Hello from processor 1 of 4
Hello from processor 2 of 4
Hello from processor 3 of 4
```

---

## 📘 Notas Finales

* Usa redes LAN estables y verifica que los cortafuegos (firewalls) permitan SSH.
* En Raspberry Pi, habilita SSH desde `raspi-config` si no está activo:

  ```bash
  sudo raspi-config
  → Interfacing Options → SSH → Enable
  ```
* En sistemas modernos, se recomienda usar **`mpirun --oversubscribe`** si ejecutas más procesos que núcleos disponibles.

---

¿Deseas que te prepare la **versión extendida en formato README.md** con numeración, emojis y comandos listos para copiar? Puedo dejarla con bloques plegables (`<details>`) estilo documentación profesional.
