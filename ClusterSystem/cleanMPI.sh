#!/bin/bash
# Script de limpieza completa de MPICH/Hydra

# Detener cualquier proceso MPI en ejecución
killall -9 mpiexec mpirun hydra_pmi_proxy 2>/dev/null

# Eliminar instalación de MPICH en /usr/local
sudo rm -rf /usr/local/mpich*
sudo rm -rf /opt/mpich*
sudo rm -f /usr/local/bin/mpi*
sudo rm -f /usr/local/bin/hydra*

# Limpiar configuraciones de ldconfig
sudo rm -f /etc/ld.so.conf.d/mpich*.conf
sudo rm -f /etc/ld.so.conf.d/mpi*.conf

# Limpiar variables de entorno del .bashrc
cp ~/.bashrc ~/.bashrc.backup
sed -i '/mpich/Id' ~/.bashrc
sed -i '/MPICH/Id' ~/.bashrc
sed -i '/HYDRA/Id' ~/.bashrc

# Limpiar paquetes del sistema si fueron instalados
sudo apt-get remove --purge -y mpich libmpich-dev 2>/dev/null
sudo apt-get remove --purge -y libmpich12 2>/dev/null
sudo apt-get autoremove -y

# Limpiar directorios de compilación
rm -rf ~/mpich*
rm -rf ~/hydra*

# Actualizar cache de bibliotecas
sudo ldconfig

# Recargar bashrc limpio
source ~/.bashrc

echo "✓ Limpieza de MPICH completada"
echo "✓ Backup de .bashrc guardado en ~/.bashrc.backup"