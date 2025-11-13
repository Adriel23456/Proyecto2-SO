Se aplica:

TODOS:
------------------------------------------
cd ~/Documents/Proyecto2-SO
git fetch --all
git reset --hard origin/master
git clean -fd
------------------------------------------

Slaves:
------------------------------------------
cd ~/Documents/Proyecto2-SO/MainSystem/Slave
make
------------------------------------------

MASTER:
------------------------------------------
cd ~/Documents/Proyecto2-SO/MainSystem/Master
make

chmod +x run_mpi_safe.sh
sudo cp run_mpi_safe.sh /usr/local/bin/mpirun-safe
sudo chmod +x /usr/local/bin/mpirun-safe

mpirun-safe ~/Documents/Proyecto2-SO/ImagesExamples/image1.png

scp result.png result_histogram.cvc result_histogram.png \
    adriel@adriel-System:~/Documents/Proyecto2-SO/MainSystem/Slave/
------------------------------------------
