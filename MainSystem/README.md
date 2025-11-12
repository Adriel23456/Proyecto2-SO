Se aplica:

TODOS:
-----------------------
cd ~/Documents/Proyecto2-SO
git pull origin master
-----------------------

Slaves:
-----------------------
cd ~/Documents/Proyecto2-SO/MainSystem/Slave
make
-----------------------

MASTER:
-----------------------
cd ~/Documents/Proyecto2-SO/MainSystem/Master
make
chmod +x run_mpi_safe.sh
sudo cp run_mpi_safe.sh /usr/local/bin/mpirun-safe
sudo chmod +x /usr/local/bin/mpirun-safe
mpirun-safe ~/Documents/Proyecto2-SO/MainSystem/Master/image.png
-----------------------