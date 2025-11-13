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

# Monitorear mientras el programa corre
#!/bin/bash

# === FUNCIÓN PARA MEDIR USO DE CPU EN UN HOST (LOCAL O REMOTO) ===
get_cpu_usage() {
    host="$1"

    if [ "$host" = "localhost" ]; then
        top_out=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}')
    else
        top_out=$(ssh "$host" "top -bn1 | grep 'Cpu(s)'" 2>/dev/null | awk '{print \$2}')
    fi

    if [[ -z "$top_out" ]]; then
        echo "N/A"
    else
        echo "$top_out"
    fi
}

# Monitorear mientras el programa corre
echo "Ejecuta tu programa MPI en otra terminal"
echo "Presiona Ctrl+C para detener el monitoreo"
echo

while true; do
    clear
    echo "$(date '+%H:%M:%S') - USO DE CPU EN CLUSTER"
    echo "═══════════════════════════════════════════════════════════"
    
    for host in localhost slave1 slave2 slave3; do
        usage=$(get_cpu_usage "$host")

        printf "  %-15s: " "$host"

        if [[ "$usage" != "N/A" ]]; then
            usage_int=${usage%.*}

            if [[ "$usage_int" =~ ^[0-9]+$ ]]; then
                # Colorear según nivel
                if [ "$usage_int" -gt 70 ]; then
                    echo -e "\033[0;32m${usage}%\033[0m ████████"
                elif [ "$usage_int" -gt 30 ]; then
                    echo -e "\033[1;33m${usage}%\033[0m ████"
                else
                    echo -e "\033[0;31m${usage}%\033[0m ██"
                fi
            else
                echo -e "\033[0;90mError CPU\033[0m"
            fi
        else
            echo -e "\033[0;90mNo disponible\033[0m"
        fi
    done

    echo "═══════════════════════════════════════════════════════════"
    sleep 1
done
------------------------------------------