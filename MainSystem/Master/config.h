/***************************************************************************//**
*  \file       config.h
*  \brief      Configuraciones globales y constantes del sistema
*  \details    Define máscaras Sobel, tags MPI, y parámetros del sistema
*******************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ============================================================================
// CONFIGURACIÓN DEL SISTEMA
// ============================================================================

#define MAX_PATH_LENGTH 512
#define MAX_FILENAME_LENGTH 256

// Tags MPI para comunicación
#define TAG_IMAGE_SECTION    100
#define TAG_MASK_SOBEL       101
#define TAG_SECTION_INFO     102
#define TAG_RESULT_SECTION   200

// ============================================================================
// MÁSCARAS SOBEL (Hardcoded - 3x3)
// ============================================================================

// Máscara Sobel para detección de bordes en X
static const float SOBEL_X[3][3] = {
    {-1.0f,  0.0f,  1.0f},
    {-2.0f,  0.0f,  2.0f},
    {-1.0f,  0.0f,  1.0f}
};

// Máscara Sobel para detección de bordes en Y
static const float SOBEL_Y[3][3] = {
    {-1.0f, -2.0f, -1.0f},
    { 0.0f,  0.0f,  0.0f},
    { 1.0f,  2.0f,  1.0f}
};

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

// Información de una sección de imagen
typedef struct {
    int section_id;      // ID de la sección (0, 1, 2, ...)
    int start_row;       // Fila inicial en la imagen completa
    int num_rows;        // Número de filas en esta sección
    int width;           // Ancho de la sección (igual al ancho total)
} SectionInfo;

// Imagen en escala de grises
typedef struct {
    uint8_t *data;       // Datos de la imagen (1 byte por pixel)
    int width;           // Ancho en píxeles
    int height;          // Alto en píxeles
    int channels;        // Número de canales (1 para grayscale)
} GrayscaleImage;

// ============================================================================
// CONFIGURACIÓN DEL HISTOGRAMA
// ============================================================================

#define HISTOGRAM_BINS 256        // Número de bins (0-255)
#define LCD_WIDTH  240            // Ancho del display para .cvc
#define LCD_HEIGHT 320            // Alto del display para .cvc

#endif // CONFIG_H