#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int world_size, world_rank, name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    char hostname[256];
    
    // Inicializar MPI
    MPI_Init(&argc, &argv);
    
    // Obtener información del proceso
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Get_processor_name(processor_name, &name_len);
    
    // Obtener hostname del sistema
    gethostname(hostname, sizeof(hostname));
    
    // Detectar arquitectura
    #ifdef __arm__
        const char* arch = "ARM";
    #elif __x86_64__
        const char* arch = "x86_64";
    #elif __i386__
        const char* arch = "x86_32";
    #else
        const char* arch = "Unknown";
    #endif
    
    // Imprimir mensaje desde proceso externo
    printf("Hola desde proceso externo %d de %d total | Host: %s | Arch: %s | PID: %d\n", 
           world_rank, world_size, processor_name, arch, getpid());
    
    // Sincronizar todos los procesos
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Solo el proceso 0 imprime resumen
    if (world_rank == 0) {
        printf("\n=== Ejecución MPI Heterogénea Completada ===\n");
        printf("Total de procesos: %d\n", world_size);
    }
    
    // Finalizar MPI
    MPI_Finalize();
    return 0;
}