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
