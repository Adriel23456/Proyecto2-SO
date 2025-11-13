/***************************************************************************//**
*  \file       image_utils.c
*  \brief      Implementación de utilidades de imagen
*  \details    Usa stb_image.h y stb_image_write.h para I/O de imágenes
*******************************************************************************/

#include "image_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// STB IMAGE LIBRARY (single-header libraries)
// ============================================================================

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ============================================================================
// IMPLEMENTACIÓN: Carga y Conversión
// ============================================================================

GrayscaleImage* load_image_grayscale(const char *filename) {
    int width, height, channels;
    
    printf("[MASTER] Cargando imagen: %s\n", filename);
    
    // Cargar imagen (stb_image maneja PNG, JPG, etc.)
    unsigned char *img_data = stbi_load(filename, &width, &height, &channels, 0);
    
    if (!img_data) {
        fprintf(stderr, "[ERROR] No se pudo cargar la imagen: %s\n", filename);
        fprintf(stderr, "[ERROR] Razón: %s\n", stbi_failure_reason());
        return NULL;
    }
    
    printf("[MASTER] Imagen cargada: %dx%d, %d canales\n", width, height, channels);
    
    // Crear estructura de imagen en escala de grises
    GrayscaleImage *gray_img = (GrayscaleImage*)malloc(sizeof(GrayscaleImage));
    if (!gray_img) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para GrayscaleImage\n");
        stbi_image_free(img_data);
        return NULL;
    }
    
    gray_img->width = width;
    gray_img->height = height;
    gray_img->channels = 1;  // Escala de grises = 1 canal
    
    // Asignar memoria para datos en escala de grises
    gray_img->data = (uint8_t*)malloc(width * height * sizeof(uint8_t));
    if (!gray_img->data) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para datos de imagen\n");
        free(gray_img);
        stbi_image_free(img_data);
        return NULL;
    }
    
    // Convertir a escala de grises
    printf("[MASTER] Convirtiendo a escala de grises...\n");
    
    for (int i = 0; i < width * height; i++) {
        if (channels == 1) {
            // Ya está en escala de grises
            gray_img->data[i] = img_data[i];
        } else if (channels == 3 || channels == 4) {
            // RGB o RGBA -> Grayscale usando fórmula estándar
            // Gray = 0.299*R + 0.587*G + 0.114*B
            unsigned char r = img_data[i * channels + 0];
            unsigned char g = img_data[i * channels + 1];
            unsigned char b = img_data[i * channels + 2];
            gray_img->data[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
        } else {
            // Formato desconocido, tomar primer canal
            gray_img->data[i] = img_data[i * channels];
        }
    }
    
    // Liberar imagen original
    stbi_image_free(img_data);
    
    printf("[MASTER] Conversión completada exitosamente\n");
    
    return gray_img;
}

void free_grayscale_image(GrayscaleImage *img) {
    if (img) {
        if (img->data) {
            free(img->data);
        }
        free(img);
    }
}

bool save_grayscale_image(const char *filename, const GrayscaleImage *img) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida para guardar\n");
        return false;
    }
    
    printf("[MASTER] Guardando imagen: %s (%dx%d)\n", 
           filename, img->width, img->height);
    
    // stbi_write_png espera datos en formato apropiado
    int result = stbi_write_png(filename, img->width, img->height, 1, 
                                 img->data, img->width);
    
    if (!result) {
        fprintf(stderr, "[ERROR] No se pudo guardar la imagen: %s\n", filename);
        return false;
    }
    
    printf("[MASTER] Imagen guardada exitosamente\n");
    return true;
}

// ============================================================================
// IMPLEMENTACIÓN: División de Imagen
// ============================================================================

void calculate_sections(int total_height, int num_slaves, 
                        SectionInfo *sections, int width) {
    printf("[MASTER] Dividiendo imagen de altura %d en %d secciones\n", 
           total_height, num_slaves);
    
    int base_rows = total_height / num_slaves;
    int extra_rows = total_height % num_slaves;
    
    int current_row = 0;
    
    for (int i = 0; i < num_slaves; i++) {
        sections[i].section_id = i;
        sections[i].start_row = current_row;
        sections[i].width = width;
        
        // El último slave toma las filas extras (si las hay)
        if (i == num_slaves - 1) {
            sections[i].num_rows = base_rows + extra_rows;
        } else {
            sections[i].num_rows = base_rows;
        }
        
        printf("[MASTER]   Sección %d: filas %d-%d (%d filas)\n",
               i, sections[i].start_row, 
               sections[i].start_row + sections[i].num_rows - 1,
               sections[i].num_rows);
        
        current_row += sections[i].num_rows;
    }
}

GrayscaleImage* extract_section(const GrayscaleImage *original, 
                                 const SectionInfo *section) {
    if (!original || !section) {
        return NULL;
    }
    
    // Crear nueva imagen para la sección
    GrayscaleImage *section_img = (GrayscaleImage*)malloc(sizeof(GrayscaleImage));
    if (!section_img) {
        return NULL;
    }
    
    section_img->width = section->width;
    section_img->height = section->num_rows;
    section_img->channels = 1;
    
    // Asignar memoria para los datos
    int section_size = section->width * section->num_rows;
    section_img->data = (uint8_t*)malloc(section_size * sizeof(uint8_t));
    if (!section_img->data) {
        free(section_img);
        return NULL;
    }
    
    // Copiar datos de la sección
    for (int row = 0; row < section->num_rows; row++) {
        int src_row = section->start_row + row;
        int src_offset = src_row * original->width;
        int dst_offset = row * section->width;
        
        memcpy(section_img->data + dst_offset,
               original->data + src_offset,
               section->width * sizeof(uint8_t));
    }
    
    return section_img;
}

GrayscaleImage* reconstruct_image(GrayscaleImage **sections,
                                   const SectionInfo *section_infos,
                                   int num_sections,
                                   int width,
                                   int height) {
    printf("[MASTER] Reconstruyendo imagen completa (%dx%d)\n", width, height);

    // Crear imagen completa
    GrayscaleImage *full_img = (GrayscaleImage*)malloc(sizeof(GrayscaleImage));
    if (!full_img) {
        return NULL;
    }

    full_img->width = width;
    full_img->height = height;
    full_img->channels = 1;

    full_img->data = (uint8_t*)malloc(width * height * sizeof(uint8_t));
    if (!full_img->data) {
        free(full_img);
        return NULL;
    }

    // Inicializar por si acaso (no es estrictamente necesario)
    memset(full_img->data, 0, width * height * sizeof(uint8_t));

    // 1) Copiar cada sección a su posición correspondiente
    for (int i = 0; i < num_sections; i++) {
        if (!sections[i]) {
            fprintf(stderr, "[ERROR] Sección %d es NULL\n", i);
            continue;
        }

        const SectionInfo *info = &section_infos[i];

        printf("[MASTER]   Copiando sección %d (filas %d-%d)\n",
               i, info->start_row, info->start_row + info->num_rows - 1);

        for (int row = 0; row < info->num_rows; row++) {
            int dst_row = info->start_row + row;
            if (dst_row < 0 || dst_row >= height) {
                continue;
            }

            int dst_offset = dst_row * width;
            int src_offset = row * info->width;

            memcpy(full_img->data + dst_offset,
                   sections[i]->data + src_offset,
                   info->width * sizeof(uint8_t));
        }
    }

    // 2) Corregir la línea negra entre secciones
    //
    //   Cada slave pone un borde de 1 píxel negro en la parte superior e
    //   inferior de su sección. Eso hace que, al pegar las secciones, quede
    //   una línea horizontal negra en la frontera.
    //
    //   Para cada frontera entre secciones i (arriba) y i+1 (abajo):
    //     - fila_bottom = última fila de la sección i
    //     - fila_top    = primera fila de la sección i+1
    //   Reemplazamos ambas por sus vecinas interiores para “borrar” el borde.

    for (int i = 0; i < num_sections - 1; i++) {
        const SectionInfo *info_top    = &section_infos[i];
        const SectionInfo *info_bottom = &section_infos[i + 1];

        int bottom_row = info_top->start_row + info_top->num_rows - 1; // borde inferior de arriba
        int top_row    = info_bottom->start_row;                        // borde superior de abajo

        // Corregir borde inferior de la sección superior
        if (bottom_row > 0 && bottom_row < height) {
            memcpy(full_img->data + bottom_row * width,
                   full_img->data + (bottom_row - 1) * width,
                   width * sizeof(uint8_t));
        }

        // Corregir borde superior de la sección inferior
        if (top_row >= 0 && top_row < height - 1) {
            memcpy(full_img->data + top_row * width,
                   full_img->data + (top_row + 1) * width,
                   width * sizeof(uint8_t));
        }
    }

    printf("[MASTER] Reconstrucción completada\n");

    return full_img;
}