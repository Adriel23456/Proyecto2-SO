/***************************************************************************//**
*  \file       image_io.c
*  \brief      Implementación de I/O de imágenes
*******************************************************************************/

#include "image_io.h"
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool save_grayscale_image(const char *filename, const GrayscaleImage *img) {
    if (!img || !img->data) {
        fprintf(stderr, "[SLAVE ERROR] Imagen inválida para guardar\n");
        return false;
    }
    
    printf("[SLAVE] Guardando imagen: %s (%dx%d)\n", 
           filename, img->width, img->height);
    
    int result = stbi_write_png(filename, img->width, img->height, 1, 
                                 img->data, img->width);
    
    if (!result) {
        fprintf(stderr, "[SLAVE ERROR] No se pudo guardar la imagen: %s\n", filename);
        return false;
    }
    
    printf("[SLAVE] ✓ Imagen guardada exitosamente\n");
    return true;
}

void free_grayscale_image(GrayscaleImage *img) {
    if (img) {
        if (img->data) {
            free(img->data);
        }
        free(img);
    }
}