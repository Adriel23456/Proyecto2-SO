/***************************************************************************//**
*  \file       main.c (SLAVE)
*  \brief      Programa principal del Slave
*  \details    Recibe sección de imagen, aplica filtro Sobel y reenvía resultado
*  
*  FLUJO:
*  1. Inicializar MPI
*  2. Verificar que NO es el master (rank != 0)
*  3. Recibir máscara Sobel desde master
*  4. Recibir información de sección
*  5. Recibir datos de la sección de imagen
*  6. Aplicar filtro Sobel
*  7. Guardar sección procesada localmente (section.png)
*  8. Reenviar sección procesada al master
*  9. Finalizar
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include "config.h"
#include "sobel_filter.h"
#include "image_io.h"

// ============================================================================
// FUNCIONES DE COMUNICACIÓN MPI
// ============================================================================

/**
 * \brief Recibe las máscaras Sobel desde el master
 */
bool receive_sobel_mask(SobelMask *mask) {
    MPI_Status status;
    float sobel_x_flat[9];
    float sobel_y_flat[9];
    
    printf("[SLAVE] Esperando máscara Sobel desde master...\n");
    
    // Recibir Sobel X
    MPI_Recv(sobel_x_flat, 9, MPI_FLOAT, 0, TAG_MASK_SOBEL, 
             MPI_COMM_WORLD, &status);
    
    // Recibir Sobel Y
    MPI_Recv(sobel_y_flat, 9, MPI_FLOAT, 0, TAG_MASK_SOBEL,
             MPI_COMM_WORLD, &status);
    
    // Convertir de 1D a 2D
    int idx = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mask->sobel_x[i][j] = sobel_x_flat[idx];
            mask->sobel_y[i][j] = sobel_y_flat[idx];
            idx++;
        }
    }
    
    printf("[SLAVE] ✓ Máscara Sobel recibida\n");
    return true;
}

/**
 * \brief Recibe información de la sección desde el master
 */
bool receive_section_info(SectionInfo *section_info) {
    MPI_Status status;
    int info_data[4];
    
    printf("[SLAVE] Esperando información de sección desde master...\n");
    
    MPI_Recv(info_data, 4, MPI_INT, 0, TAG_SECTION_INFO,
             MPI_COMM_WORLD, &status);
    
    section_info->section_id = info_data[0];
    section_info->start_row = info_data[1];
    section_info->num_rows = info_data[2];
    section_info->width = info_data[3];
    
    printf("[SLAVE] ✓ Información recibida: Sección ID=%d, filas=%d-%d, ancho=%d\n",
           section_info->section_id,
           section_info->start_row,
           section_info->start_row + section_info->num_rows - 1,
           section_info->width);
    
    return true;
}

/**
 * \brief Recibe los datos de la sección de imagen desde el master
 */
GrayscaleImage* receive_image_section() {
    MPI_Status status;
    int size_info[2];
    
    printf("[SLAVE] Esperando datos de imagen desde master...\n");
    
    // Recibir tamaño
    MPI_Recv(size_info, 2, MPI_INT, 0, TAG_IMAGE_SECTION,
             MPI_COMM_WORLD, &status);
    
    int width = size_info[0];
    int height = size_info[1];
    
    printf("[SLAVE] Tamaño de sección: %dx%d\n", width, height);
    
    // Crear imagen
    GrayscaleImage *img = (GrayscaleImage*)malloc(sizeof(GrayscaleImage));
    if (!img) {
        fprintf(stderr, "[SLAVE ERROR] No se pudo asignar memoria para imagen\n");
        return NULL;
    }
    
    img->width = width;
    img->height = height;
    img->channels = 1;
    
    int data_size = width * height;
    img->data = (uint8_t*)malloc(data_size * sizeof(uint8_t));
    if (!img->data) {
        fprintf(stderr, "[SLAVE ERROR] No se pudo asignar memoria para datos de imagen\n");
        free(img);
        return NULL;
    }
    
    // Recibir datos
    MPI_Recv(img->data, data_size, MPI_UNSIGNED_CHAR, 0, TAG_IMAGE_SECTION,
             MPI_COMM_WORLD, &status);
    
    printf("[SLAVE] ✓ Datos de imagen recibidos (%d bytes)\n", data_size);
    
    return img;
}

/**
 * \brief Envía información de sección procesada al master
 */
bool send_section_info(const SectionInfo *section_info) {
    int info_data[4] = {
        section_info->section_id,
        section_info->start_row,
        section_info->num_rows,
        section_info->width
    };
    
    printf("[SLAVE] Enviando información de sección al master...\n");
    
    MPI_Send(info_data, 4, MPI_INT, 0, TAG_RESULT_SECTION, MPI_COMM_WORLD);
    
    printf("[SLAVE] ✓ Información enviada\n");
    return true;
}

/**
 * \brief Envía sección de imagen procesada al master
 */
bool send_image_section(const GrayscaleImage *img) {
    if (!img || !img->data) {
        fprintf(stderr, "[SLAVE ERROR] Imagen inválida para enviar\n");
        return false;
    }
    
    int size_info[2] = { img->width, img->height };
    int data_size = img->width * img->height;
    
    printf("[SLAVE] Enviando imagen procesada al master (%d bytes)...\n", data_size);
    
    // Enviar tamaño
    MPI_Send(size_info, 2, MPI_INT, 0, TAG_RESULT_SECTION, MPI_COMM_WORLD);
    
    // Enviar datos
    MPI_Send(img->data, data_size, MPI_UNSIGNED_CHAR, 0, TAG_RESULT_SECTION,
             MPI_COMM_WORLD);
    
    printf("[SLAVE] ✓ Imagen enviada al master\n");
    return true;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

int main(int argc, char** argv) {
    int world_rank, world_size;
    double start_time, end_time;
    
    // ========================================================================
    // PASO 1: Inicializar MPI y OpenMP
    // ========================================================================
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    start_time = MPI_Wtime();

    #ifdef _OPENMP
    #include <omp.h>
    #include <unistd.h>

    // Configurar threads al 75% de cores disponibles
    int configure_openmp_threads() {
        int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        int num_threads = (num_cores * 75) / 100;  // 75% de los cores
        
        if (num_threads < 1) num_threads = 1;
        
        // Configurar OpenMP
        omp_set_num_threads(num_threads);
        
        printf("[SLAVE] Sistema tiene %d cores, configurando %d threads OpenMP (75%%)\n", 
            num_cores, num_threads);
        
        return num_threads;
    }
    #endif

    // En el main del slave, después de verificar que es un slave:
    #ifdef _OPENMP
        configure_openmp_threads();
    #endif
    
    // ========================================================================
    // PASO 2: Verificar que este proceso NO es el Master
    // ========================================================================
    
    if (world_rank == 0) {
        // Este es el master, finalizar silenciosamente
        MPI_Finalize();
        return 0;
    }
    
    // A partir de aquí, solo los slaves ejecutan
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  SLAVE %d INICIADO\n", world_rank);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Total de procesos: %d\n", world_size);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    // ========================================================================
    // PASO 3: Recibir máscara Sobel
    // ========================================================================
    
    SobelMask sobel_mask;
    if (!receive_sobel_mask(&sobel_mask)) {
        fprintf(stderr, "[SLAVE ERROR] Fallo al recibir máscara Sobel\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    // ========================================================================
    // PASO 4: Recibir información de sección
    // ========================================================================
    
    SectionInfo section_info;
    if (!receive_section_info(&section_info)) {
        fprintf(stderr, "[SLAVE ERROR] Fallo al recibir información de sección\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    // ========================================================================
    // PASO 5: Recibir datos de imagen
    // ========================================================================
    
    GrayscaleImage *input_section = receive_image_section();
    if (!input_section) {
        fprintf(stderr, "[SLAVE ERROR] Fallo al recibir datos de imagen\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    printf("\n");
    
    // ========================================================================
    // PASO 6: Aplicar filtro Sobel
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  APLICANDO FILTRO SOBEL\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    GrayscaleImage *output_section = apply_sobel_filter(input_section, &sobel_mask);
    
    if (!output_section) {
        fprintf(stderr, "[SLAVE ERROR] Fallo al aplicar filtro Sobel\n");
        free_grayscale_image(input_section);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    printf("\n");
    
    // ========================================================================
    // PASO 7: Guardar sección procesada localmente
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  GUARDANDO SECCIÓN PROCESADA\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    char output_path[MAX_PATH_LENGTH];
    snprintf(output_path, sizeof(output_path),
             "%s/Documents/Proyecto2-SO/MainSystem/Slave/section.png",
             getenv("HOME"));
    
    if (!save_grayscale_image(output_path, output_section)) {
        fprintf(stderr, "[SLAVE ERROR] No se pudo guardar imagen localmente\n");
        // Continuar de todas formas (no es crítico)
    } else {
        printf("[SLAVE] ✓ Sección guardada en: %s\n", output_path);
    }
    
    printf("\n");
    
    // ========================================================================
    // PASO 8: Reenviar sección procesada al master
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ENVIANDO RESULTADO AL MASTER\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    if (!send_section_info(&section_info)) {
        fprintf(stderr, "[SLAVE ERROR] Fallo al enviar información al master\n");
        free_grayscale_image(input_section);
        free_grayscale_image(output_section);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    if (!send_image_section(output_section)) {
        fprintf(stderr, "[SLAVE ERROR] Fallo al enviar imagen al master\n");
        free_grayscale_image(input_section);
        free_grayscale_image(output_section);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    printf("\n");
    
    // ========================================================================
    // PASO 9: Limpieza y finalización
    // ========================================================================
    
    free_grayscale_image(input_section);
    free_grayscale_image(output_section);
    
    end_time = MPI_Wtime();
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✓ SLAVE %d COMPLETADO EXITOSAMENTE\n", world_rank);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Tiempo de procesamiento: %.2f segundos\n", end_time - start_time);
    printf("  Sección procesada: ID %d\n", section_info.section_id);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    MPI_Finalize();
    return 0;
}