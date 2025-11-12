/***************************************************************************//**
*  \file       image_utils.h
*  \brief      Declaraciones para manejo de imágenes
*  \details    Carga, conversión a escala de grises, división y guardado
*******************************************************************************/

#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include "config.h"
#include <stdbool.h>

// ============================================================================
// FUNCIONES DE CARGA Y CONVERSIÓN
// ============================================================================

/**
 * \brief Carga una imagen y la convierte a escala de grises
 * \param filename Ruta de la imagen a cargar
 * \return Estructura GrayscaleImage con la imagen cargada, o NULL si falla
 */
GrayscaleImage* load_image_grayscale(const char *filename);

/**
 * \brief Libera memoria de una imagen en escala de grises
 * \param img Puntero a la imagen a liberar
 */
void free_grayscale_image(GrayscaleImage *img);

/**
 * \brief Guarda una imagen en escala de grises como PNG
 * \param filename Nombre del archivo de salida
 * \param img Imagen a guardar
 * \return true si se guardó correctamente, false si hubo error
 */
bool save_grayscale_image(const char *filename, const GrayscaleImage *img);

// ============================================================================
// FUNCIONES DE DIVISIÓN Y RECONSTRUCCIÓN
// ============================================================================

/**
 * \brief Calcula cómo dividir la imagen entre N slaves
 * \param total_height Alto total de la imagen
 * \param num_slaves Número de slaves disponibles
 * \param sections Array donde se guardarán las secciones (debe tener espacio para num_slaves)
 * \param width Ancho de la imagen
 */
void calculate_sections(int total_height, int num_slaves, SectionInfo *sections, int width);

/**
 * \brief Extrae una sección de la imagen original
 * \param original Imagen original completa
 * \param section Información de la sección a extraer
 * \return Nueva imagen con solo la sección especificada
 */
GrayscaleImage* extract_section(const GrayscaleImage *original, const SectionInfo *section);

/**
 * \brief Reconstruye la imagen completa a partir de secciones
 * \param sections Array de secciones procesadas
 * \param section_infos Array con información de cada sección
 * \param num_sections Número de secciones
 * \param width Ancho de la imagen completa
 * \param height Alto de la imagen completa
 * \return Imagen completa reconstruida
 */
GrayscaleImage* reconstruct_image(GrayscaleImage **sections, 
                                   const SectionInfo *section_infos,
                                   int num_sections,
                                   int width, 
                                   int height);

#endif // IMAGE_UTILS_H