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

# Monitorear el sistema local de Raspberry Pi 2
------------------------------------------
#!/bin/bash

get_cpu_usage_per_core() {
    prev=()
    curr=()

    # ======== Lectura inicial ========
    while read -r line; do
        if [[ "$line" =~ ^cpu[0-9]+ ]]; then
            prev+=("$line")
        fi
    done < /proc/stat

    sleep 0.5

    # ======== Lectura después ========
    while read -r line; do
        if [[ "$line" =~ ^cpu[0-9]+ ]]; then
            curr+=("$line")
        fi
    done < /proc/stat

    core_count=${#curr[@]}

    for ((i=0; i<core_count; i++)); do
        read -ra p <<< "${prev[$i]}"
        read -ra c <<< "${curr[$i]}"

        idle_prev=${p[4]}
        idle_curr=${c[4]}

        total_prev=0
        total_curr=0

        for n in "${p[@]:1}"; do total_prev=$((total_prev + n)); done
        for n in "${c[@]:1}"; do total_curr=$((total_curr + n)); done

        diff_total=$(( total_curr - total_prev ))
        diff_idle=$(( idle_curr - idle_prev ))

        usage=$(( 100 * (diff_total - diff_idle) / diff_total ))

        echo "$usage"
    done
}

# ===== MONITOR =====
while true; do
    clear
    echo "$(date '+%H:%M:%S') - USO DE CPU POR NÚCLEO"
    echo "═══════════════════════════════════════════════════════════"

    mapfile -t usages < <(get_cpu_usage_per_core)

    core=0
    for usage in "${usages[@]}"; do
        bars=$(( usage / 5 ))
        bar=$(printf "%${bars}s" | tr ' ' '█')

        if (( usage > 70 )); then color="\033[0;32m"
        elif (( usage > 30 )); then color="\033[1;33m"
        else color="\033[0;31m"
        fi

        printf "  Core %-2d: ${color}%3d%%\033[0m %s\n" "$core" "$usage" "$bar"
        ((core++))
    done

    echo "═══════════════════════════════════════════════════════════"
    sleep 1
done
------------------------------------------