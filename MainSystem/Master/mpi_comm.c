/***************************************************************************//**
*  \file       mpi_comm.c
*  \brief      Implementación de comunicación MPI
*  \details    Maneja todo el envío y recepción de datos entre master y slaves
*******************************************************************************/

#include "mpi_comm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// IMPLEMENTACIÓN: Funciones Auxiliares
// ============================================================================

int get_num_slaves(int world_size) {
    // Rank 0 es el master, el resto son slaves
    return world_size - 1;
}

void print_mpi_info(int world_rank, int world_size) {
    if (world_rank == 0) {
        printf("\n");
        printf("═══════════════════════════════════════════════════════════\n");
        printf("  SISTEMA DE PROCESAMIENTO DISTRIBUIDO DE IMÁGENES\n");
        printf("═══════════════════════════════════════════════════════════\n");
        printf("  Master Rank: %d\n", world_rank);
        printf("  Total Processes: %d\n", world_size);
        printf("  Available Slaves: %d\n", get_num_slaves(world_size));
        printf("═══════════════════════════════════════════════════════════\n\n");
    }
}

// ============================================================================
// IMPLEMENTACIÓN: Envío de Datos
// ============================================================================

bool send_sobel_mask(int slave_rank) {
    printf("[MASTER] Enviando máscara Sobel a slave %d\n", slave_rank);
    
    // Enviar máscara Sobel X (aplanada a 1D)
    float sobel_x_flat[9];
    int idx = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            sobel_x_flat[idx++] = SOBEL_X[i][j];
        }
    }
    
    // Enviar máscara Sobel Y (aplanada a 1D)
    float sobel_y_flat[9];
    idx = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            sobel_y_flat[idx++] = SOBEL_Y[i][j];
        }
    }
    
    // Enviar ambas máscaras
    MPI_Send(sobel_x_flat, 9, MPI_FLOAT, slave_rank, TAG_MASK_SOBEL, MPI_COMM_WORLD);
    MPI_Send(sobel_y_flat, 9, MPI_FLOAT, slave_rank, TAG_MASK_SOBEL, MPI_COMM_WORLD);
    
    printf("[MASTER] ✓ Máscara Sobel enviada a slave %d\n", slave_rank);
    
    return true;
}

bool send_section_info(int slave_rank, const SectionInfo *section_info) {
    printf("[MASTER] Enviando información de sección %d a slave %d\n",
           section_info->section_id, slave_rank);
    
    // Empaquetar información en un array
    int info_data[4] = {
        section_info->section_id,
        section_info->start_row,
        section_info->num_rows,
        section_info->width
    };
    
    MPI_Send(info_data, 4, MPI_INT, slave_rank, TAG_SECTION_INFO, MPI_COMM_WORLD);
    
    printf("[MASTER] ✓ Información enviada: ID=%d, filas=%d-%d, ancho=%d\n",
           section_info->section_id,
           section_info->start_row,
           section_info->start_row + section_info->num_rows - 1,
           section_info->width);
    
    return true;
}

bool send_image_section(int slave_rank, const GrayscaleImage *section) {
    if (!section || !section->data) {
        fprintf(stderr, "[ERROR] Sección de imagen inválida\n");
        return false;
    }
    
    int data_size = section->width * section->height;
    
    printf("[MASTER] Enviando %d bytes de imagen a slave %d\n", 
           data_size, slave_rank);
    
    // Enviar tamaño primero
    int size_info[2] = { section->width, section->height };
    MPI_Send(size_info, 2, MPI_INT, slave_rank, TAG_IMAGE_SECTION, MPI_COMM_WORLD);
    
    // Enviar datos de la imagen
    MPI_Send(section->data, data_size, MPI_UNSIGNED_CHAR, 
             slave_rank, TAG_IMAGE_SECTION, MPI_COMM_WORLD);
    
    printf("[MASTER] ✓ Sección de imagen enviada a slave %d\n", slave_rank);
    
    return true;
}

// ============================================================================
// IMPLEMENTACIÓN: Recepción de Datos
// ============================================================================

bool receive_section_info(int slave_rank, SectionInfo *section_info, int *actual_source) {
    MPI_Status status;
    int info_data[4];
    
    int source = (slave_rank == MPI_ANY_SOURCE) ? MPI_ANY_SOURCE : slave_rank;
    
    printf("[MASTER] Esperando información de sección desde slave...\n");
    
    MPI_Recv(info_data, 4, MPI_INT, source, TAG_RESULT_SECTION, 
             MPI_COMM_WORLD, &status);
    
    section_info->section_id = info_data[0];
    section_info->start_row = info_data[1];
    section_info->num_rows = info_data[2];
    section_info->width = info_data[3];
    
    if (actual_source) {
        *actual_source = status.MPI_SOURCE;
    }
    
    printf("[MASTER] ✓ Recibida info de sección %d desde slave %d\n",
           section_info->section_id, status.MPI_SOURCE);
    
    return true;
}

GrayscaleImage* receive_image_section(int slave_rank, const SectionInfo *section_info) {
    MPI_Status status;
    
    printf("[MASTER] Recibiendo sección %d procesada desde slave %d\n",
           section_info->section_id, slave_rank);
    
    // Recibir tamaño
    int size_info[2];
    MPI_Recv(size_info, 2, MPI_INT, slave_rank, TAG_RESULT_SECTION,
             MPI_COMM_WORLD, &status);
    
    int width = size_info[0];
    int height = size_info[1];
    
    // Crear imagen para recibir datos
    GrayscaleImage *received_img = (GrayscaleImage*)malloc(sizeof(GrayscaleImage));
    if (!received_img) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para imagen recibida\n");
        return NULL;
    }
    
    received_img->width = width;
    received_img->height = height;
    received_img->channels = 1;
    
    int data_size = width * height;
    received_img->data = (uint8_t*)malloc(data_size * sizeof(uint8_t));
    if (!received_img->data) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para datos de imagen\n");
        free(received_img);
        return NULL;
    }
    
    // Recibir datos de la imagen
    MPI_Recv(received_img->data, data_size, MPI_UNSIGNED_CHAR,
             slave_rank, TAG_RESULT_SECTION, MPI_COMM_WORLD, &status);
    
    printf("[MASTER] ✓ Sección %d recibida (%dx%d) desde slave %d\n",
           section_info->section_id, width, height, slave_rank);
    
    return received_img;
}