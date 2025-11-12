/***************************************************************************//**
*  \file       config.h
*  \brief      Configuraciones globales y constantes del sistema (SLAVE)
*  \details    Define tags MPI y estructuras de datos
*******************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ============================================================================
// CONFIGURACIÓN DEL SISTEMA
// ============================================================================

#define MAX_PATH_LENGTH 512

// Tags MPI para comunicación (deben coincidir con el master)
#define TAG_IMAGE_SECTION    100
#define TAG_MASK_SOBEL       101
#define TAG_SECTION_INFO     102
#define TAG_RESULT_SECTION   200

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

// Información de una sección de imagen
typedef struct {
    int section_id;      // ID de la sección (0, 1, 2, ...)
    int start_row;       // Fila inicial en la imagen completa
    int num_rows;        // Número de filas en esta sección
    int width;           // Ancho de la sección
} SectionInfo;

// Imagen en escala de grises
typedef struct {
    uint8_t *data;       // Datos de la imagen (1 byte por pixel)
    int width;           // Ancho en píxeles
    int height;          // Alto en píxeles
    int channels;        // Número de canales (1 para grayscale)
} GrayscaleImage;

// Máscaras Sobel
typedef struct {
    float sobel_x[3][3];  // Máscara Sobel X
    float sobel_y[3][3];  // Máscara Sobel Y
} SobelMask;

#endif // CONFIG_H