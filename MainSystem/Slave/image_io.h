/***************************************************************************//**
*  \file       image_io.h
*  \brief      Declaraciones para I/O de imágenes del slave
*  \details    Guardado de imágenes en formato PNG
*******************************************************************************/

#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include "config.h"
#include <stdbool.h>

/**
 * \brief Guarda una imagen en escala de grises como PNG
 * \param filename Nombre del archivo de salida
 * \param img Imagen a guardar
 * \return true si se guardó correctamente, false si hubo error
 */
bool save_grayscale_image(const char *filename, const GrayscaleImage *img);

/**
 * \brief Libera memoria de una imagen en escala de grises
 * \param img Puntero a la imagen a liberar
 */
void free_grayscale_image(GrayscaleImage *img);

#endif // IMAGE_IO_H