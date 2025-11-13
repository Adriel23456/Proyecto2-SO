/***************************************************************************//**
*  \file       sobel_filter.c
*  \brief      Implementación del filtro Sobel con paralelización OpenMP
*  \details    Aplica operadores Sobel X e Y y calcula magnitud del gradiente
*******************************************************************************/

#include "sobel_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * \brief Aplica convolución 3x3 en una posición específica
 */
static float apply_convolution_3x3(const GrayscaleImage *img, 
                                   int x, int y, 
                                   const float kernel[3][3]) {
    float sum = 0.0f;
    
    // Aplicar kernel 3x3
    for (int ky = -1; ky <= 1; ky++) {
        for (int kx = -1; kx <= 1; kx++) {
            int img_x = x + kx;
            int img_y = y + ky;
            
            // Verificar límites
            if (img_x >= 0 && img_x < img->width && 
                img_y >= 0 && img_y < img->height) {
                
                int pixel_idx = img_y * img->width + img_x;
                float pixel_value = (float)img->data[pixel_idx];
                float kernel_value = kernel[ky + 1][kx + 1];
                
                sum += pixel_value * kernel_value;
            }
        }
    }
    
    return sum;
}

/**
 * \brief Clampea un valor al rango 0-255
 */
static uint8_t clamp_to_byte(float value) {
    if (value < 0.0f) return 0;
    if (value > 255.0f) return 255;
    return (uint8_t)value;
}

// ============================================================================
// IMPLEMENTACIÓN: Filtro Sobel con OpenMP
// ============================================================================

GrayscaleImage* apply_sobel_filter(const GrayscaleImage *img, const SobelMask *mask) {
    if (!img || !img->data || !mask) {
        fprintf(stderr, "[SLAVE ERROR] Datos inválidos para aplicar filtro Sobel\n");
        return NULL;
    }
    
    printf("[SLAVE] Aplicando filtro Sobel a imagen %dx%d\n", img->width, img->height);
    
    // Mostrar información de OpenMP
    #ifdef _OPENMP
    #pragma omp parallel
    {
        #pragma omp single
        {
            printf("[SLAVE] OpenMP activado: usando %d threads\n", omp_get_num_threads());
        }
    }
    #else
    printf("[SLAVE] OpenMP NO activado (ejecución secuencial)\n");
    #endif
    
    // Crear imagen de salida
    GrayscaleImage *output = (GrayscaleImage*)malloc(sizeof(GrayscaleImage));
    if (!output) {
        fprintf(stderr, "[SLAVE ERROR] No se pudo asignar memoria para imagen de salida\n");
        return NULL;
    }
    
    output->width = img->width;
    output->height = img->height;
    output->channels = 1;
    
    int total_pixels = img->width * img->height;
    output->data = (uint8_t*)calloc(total_pixels, sizeof(uint8_t));
    if (!output->data) {
        fprintf(stderr, "[SLAVE ERROR] No se pudo asignar memoria para datos de salida\n");
        free(output);
        return NULL;
    }
    
    // Variables para progreso (compartidas entre threads)
    int processed_pixels = 0;
    int total_inner_pixels = (img->width - 2) * (img->height - 2);
    int last_progress = 0;
    
    // PARALELIZACIÓN CON OpenMP
    #ifdef _OPENMP
    #pragma omp parallel
    {
        // Cada thread procesa un subconjunto de filas
        #pragma omp for schedule(dynamic, 10) nowait
        for (int y = 1; y < img->height - 1; y++) {
            for (int x = 1; x < img->width - 1; x++) {
                // Aplicar Sobel X (gradiente horizontal)
                float gx = apply_convolution_3x3(img, x, y, mask->sobel_x);

                // Aplicar Sobel Y (gradiente vertical)
                float gy = apply_convolution_3x3(img, x, y, mask->sobel_y);

                // Calcular magnitud del gradiente: sqrt(Gx² + Gy²)
                float magnitude = sqrtf(gx * gx + gy * gy);

                // Normalizar y guardar resultado
                int out_idx = y * img->width + x;
                output->data[out_idx] = clamp_to_byte(magnitude);
            }

            // Actualizar progreso (thread-safe)
            #pragma omp atomic
            processed_pixels += (img->width - 2);

            if (total_inner_pixels > 0) {
                int progress = (processed_pixels * 100) / total_inner_pixels;

                // Solo un hilo a la vez evalúa/imprime
                #pragma omp critical
                {
                    // Opcional: solo el thread 0
                    // if (omp_get_thread_num() == 0) {

                    if (progress >= last_progress + 10) {
                        printf("[SLAVE]   Progreso: %d%%\n", progress);
                        fflush(stdout);
                        last_progress = progress;
                    }

                    // }
                }
            }
        }
    }
    #else
    // Versión secuencial (sin OpenMP)
    for (int y = 1; y < img->height - 1; y++) {
        for (int x = 1; x < img->width - 1; x++) {
            // Aplicar Sobel X (gradiente horizontal)
            float gx = apply_convolution_3x3(img, x, y, mask->sobel_x);
            
            // Aplicar Sobel Y (gradiente vertical)
            float gy = apply_convolution_3x3(img, x, y, mask->sobel_y);
            
            // Calcular magnitud del gradiente: sqrt(Gx² + Gy²)
            float magnitude = sqrtf(gx * gx + gy * gy);
            
            // Normalizar y guardar resultado
            int out_idx = y * img->width + x;
            output->data[out_idx] = clamp_to_byte(magnitude);
            
            processed_pixels++;
        }
        
        // Mostrar progreso cada 10%
        if (total_inner_pixels > 0 && 
            processed_pixels % (total_inner_pixels / 10 + 1) == 0) {
            int progress = (processed_pixels * 100) / total_inner_pixels;
            printf("[SLAVE]   Progreso: %d%%\r", progress);
            fflush(stdout);
        }
    }
    #endif
    
    printf("[SLAVE]   Progreso: 100%%\n");
    printf("[SLAVE] ✓ Filtro Sobel aplicado exitosamente\n");
    
    // Los bordes quedan en negro (ya inicializados a 0 con calloc)
    
    return output;
}