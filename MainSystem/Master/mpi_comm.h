/***************************************************************************//**
*  \file       mpi_comm.h
*  \brief      Declaraciones para comunicación MPI
*  \details    Envío y recepción de secciones de imagen y máscaras
*******************************************************************************/

#ifndef MPI_COMM_H
#define MPI_COMM_H

#include "config.h"
#include <mpi.h>
#include <stdbool.h>

// ============================================================================
// FUNCIONES DE COMUNICACIÓN MPI
// ============================================================================

/**
 * \brief Verifica cuántos slaves están disponibles
 * \param world_size Tamaño total del mundo MPI
 * \return Número de slaves (world_size - 1, ya que rank 0 es master)
 */
int get_num_slaves(int world_size);

/**
 * \brief Envía la máscara Sobel a un slave específico
 * \param slave_rank Rank del slave destinatario
 * \return true si se envió correctamente
 */
bool send_sobel_mask(int slave_rank);

/**
 * \brief Envía información de sección a un slave
 * \param slave_rank Rank del slave destinatario
 * \param section_info Información de la sección
 * \return true si se envió correctamente
 */
bool send_section_info(int slave_rank, const SectionInfo *section_info);

/**
 * \brief Envía los datos de una sección de imagen a un slave
 * \param slave_rank Rank del slave destinatario
 * \param section Sección de imagen a enviar
 * \return true si se envió correctamente
 */
bool send_image_section(int slave_rank, const GrayscaleImage *section);

/**
 * \brief Recibe información de sección procesada desde un slave
 * \param slave_rank Rank del slave que envía (puede ser MPI_ANY_SOURCE)
 * \param section_info Estructura donde se guardará la información
 * \param actual_source Puntero donde se guardará el rank real del emisor
 * \return true si se recibió correctamente
 */
bool receive_section_info(int slave_rank, SectionInfo *section_info, int *actual_source);

/**
 * \brief Recibe una sección de imagen procesada desde un slave
 * \param slave_rank Rank del slave que envía
 * \param section_info Información de la sección (para saber el tamaño)
 * \return Imagen recibida o NULL si hubo error
 */
GrayscaleImage* receive_image_section(int slave_rank, const SectionInfo *section_info);

/**
 * \brief Imprime información de estado de MPI
 * \param world_rank Rank del proceso actual
 * \param world_size Tamaño del mundo MPI
 */
void print_mpi_info(int world_rank, int world_size);

#endif // MPI_COMM_H