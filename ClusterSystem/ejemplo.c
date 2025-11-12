#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int world_size, world_rank, name_len;
    char name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Get_processor_name(name, &name_len);

    printf("Hola desde el proceso %d de %d en host %s\n", world_rank, world_size, name);

    MPI_Finalize();
    return 0;
}
