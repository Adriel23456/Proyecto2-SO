/***************************************************************************//**
*  \file       histogram.h
*  \brief      Declaraciones para generación de histogramas
*  \details    Cálculo de histograma y generación de archivo .cvc
*******************************************************************************/

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ESTRUCTURA DE HISTOGRAMA
// ============================================================================

typedef struct {
    uint32_t bins[HISTOGRAM_BINS];  // Conteo de cada valor (0-255)
    int total_pixels;                // Total de píxeles analizados
    uint8_t min_value;               // Valor mínimo encontrado
    uint8_t max_value;               // Valor máximo encontrado
} Histogram;

// ============================================================================
// FUNCIONES DE HISTOGRAMA
// ============================================================================

/**
 * \brief Calcula el histograma de una imagen en escala de grises
 * \param img Imagen de entrada
 * \return Estructura Histogram con los datos calculados
 */
Histogram* calculate_histogram(const GrayscaleImage *img);

/**
 * \brief Libera memoria del histograma
 * \param hist Histograma a liberar
 */
void free_histogram(Histogram *hist);

/**
 * \brief Genera imagen PNG del histograma
 * \param hist Histograma calculado
 * \param filename Nombre del archivo de salida
 * \return true si se generó correctamente
 */
bool generate_histogram_png(const Histogram *hist, const char *filename);

/**
 * \brief Genera archivo .cvc del histograma con límites de LCD
 * \param hist Histograma calculado
 * \param filename Nombre del archivo de salida
 * \return true si se generó correctamente
 */
bool generate_histogram_cvc(const Histogram *hist, const char *filename);

/**
 * \brief Imprime estadísticas del histograma
 * \param hist Histograma a imprimir
 */
void print_histogram_stats(const Histogram *hist);

#endif // HISTOGRAM_H