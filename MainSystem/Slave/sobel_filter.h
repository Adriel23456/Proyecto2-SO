/***************************************************************************//**
*  \file       sobel_filter.h
*  \brief      Declaraciones para aplicar filtro Sobel
*  \details    Detección de bordes usando operadores Sobel
*******************************************************************************/

#ifndef SOBEL_FILTER_H
#define SOBEL_FILTER_H

#include "config.h"

/**
 * \brief Aplica el filtro Sobel a una imagen en escala de grises
 * \param img Imagen de entrada
 * \param mask Máscaras Sobel (X e Y)
 * \return Nueva imagen con el filtro aplicado
 * 
 * ALGORITMO:
 * 1. Para cada pixel (excepto bordes):
 *    - Aplicar convolución con Sobel X
 *    - Aplicar convolución con Sobel Y
 *    - Calcular magnitud: sqrt(Gx² + Gy²)
 *    - Normalizar a rango 0-255
 * 2. Bordes se mantienen en negro (0)
 */
GrayscaleImage* apply_sobel_filter(const GrayscaleImage *img, const SobelMask *mask);

#endif // SOBEL_FILTER_H