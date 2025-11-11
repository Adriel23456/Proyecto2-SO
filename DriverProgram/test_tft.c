/***************************************************************************//**
*  \file       test_tft.c
*  \details    Programa de prueba para la biblioteca libtft
*  \brief      Demuestra uso de la biblioteca para controlar el display
*
*  USO:
*  - Compilado y enlazado con libtft.a
*  - Proporciona interfaz de línea de comandos
*  - Ejecuta operaciones en el display según argumentos
*
*  EJEMPLOS:
*    sudo ./test_tft fill F800          # Llenar con rojo
*    sudo ./test_tft cvc imagen.cvc     # Cargar imagen
*    sudo ./test_tft rect 10 10 50 50 001F  # Rectángulo azul
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // Para permisos y funciones del sistema
#include "libtft.h"

/***************************************************************************//**
* \brief Muestra información de uso del programa
*******************************************************************************/
void print_usage(const char *prog)
{
    printf("TFT Display Test Program\n");
    printf("Usage: %s <command> [args]\n\n", prog);
    printf("Commands:\n");
    printf("  fill <color>               - Fill screen with RGB565 hex color\n");
    printf("                               Example: fill F800 (red)\n");
    printf("  cvc <file>                 - Load and display CVC file\n");
    printf("                               Example: cvc histogram.cvc\n");
    printf("  rect <x> <y> <w> <h> <color> - Draw filled rectangle\n");
    printf("                               Example: rect 50 50 100 80 001F\n");
    printf("\nCommon colors (RGB565):\n");
    printf("  F800 - Red\n");
    printf("  07E0 - Green\n");
    printf("  001F - Blue\n");
    printf("  FFFF - White\n");
    printf("  0000 - Black\n");
}

/***************************************************************************//**
* \brief Función principal
*
* FLUJO:
* 1. Validar argumentos
* 2. Inicializar conexión con display (tft_init)
* 3. Ejecutar comando solicitado
* 4. Cerrar conexión (tft_close)
*******************************************************************************/
int main(int argc, char *argv[])
{
    tft_handle_t *tft;
    int ret = 0;
    
    // Validar número mínimo de argumentos
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    /*
     * INICIALIZAR CONEXIÓN
     * tft_init() abre /dev/tft_device y retorna handle
     */
    printf("Initializing TFT display...\n");
    tft = tft_init();
    if (!tft) {
        fprintf(stderr, "Error: Failed to initialize TFT\n");
        fprintf(stderr, "Make sure:\n");
        fprintf(stderr, "  1. Drivers are loaded (lsmod | grep tft)\n");
        fprintf(stderr, "  2. Device exists (ls -l /dev/tft_device)\n");
        fprintf(stderr, "  3. You have permissions (run with sudo)\n");
        return 1;
    }
    
    printf("TFT initialized successfully\n");
    
    /*
     * PROCESAR COMANDO
     */
    if (strcmp(argv[1], "fill") == 0 && argc >= 3) {
        /*
         * COMANDO: fill <color>
         * Llena toda la pantalla con el color especificado
         * Color en formato hexadecimal RGB565
         */
        uint16_t color = (uint16_t)strtol(argv[2], NULL, 16);
        printf("Filling screen with color 0x%04X...\n", color);
        
        ret = tft_fill_screen(tft, color);
        if (ret < 0) {
            fprintf(stderr, "Error filling screen\n");
        } else {
            printf("Screen filled successfully\n");
        }
        
    } else if (strcmp(argv[1], "cvc") == 0 && argc >= 3) {
        /*
         * COMANDO: cvc <file>
         * Carga y muestra imagen desde archivo .cvc
         */
        printf("Loading CVC file: %s...\n", argv[2]);
        
        ret = tft_load_cvc_file(tft, argv[2]);
        if (ret < 0) {
            fprintf(stderr, "Error loading CVC file\n");
            fprintf(stderr, "Check that:\n");
            fprintf(stderr, "  1. File exists and is readable\n");
            fprintf(stderr, "  2. File format is correct (X<TAB>Y<TAB>COLOR)\n");
        } else {
            printf("Image loaded and displayed successfully\n");
        }
        
    } else if (strcmp(argv[1], "rect") == 0 && argc >= 7) {
        /*
         * COMANDO: rect <x> <y> <width> <height> <color>
         * Dibuja rectángulo relleno en posición especificada
         */
        uint16_t x = (uint16_t)atoi(argv[2]);
        uint16_t y = (uint16_t)atoi(argv[3]);
        uint16_t w = (uint16_t)atoi(argv[4]);
        uint16_t h = (uint16_t)atoi(argv[5]);
        uint16_t color = (uint16_t)strtol(argv[6], NULL, 16);
        
        printf("Drawing rectangle at (%d,%d) size %dx%d color 0x%04X...\n", 
               x, y, w, h, color);
        
        ret = tft_fill_rect(tft, x, y, w, h, color);
        if (ret < 0) {
            fprintf(stderr, "Error drawing rectangle\n");
            fprintf(stderr, "Make sure rectangle is within bounds (0-239, 0-319)\n");
        } else {
            printf("Rectangle drawn successfully\n");
        }
        
    } else {
        /*
         * COMANDO NO RECONOCIDO
         */
        fprintf(stderr, "Error: Unknown command or invalid arguments\n\n");
        print_usage(argv[0]);
        tft_close(tft);
        return 1;
    }
    
    /*
     * CERRAR CONEXIÓN
     * Siempre cerrar el dispositivo al terminar
     */
    tft_close(tft);
    
    if (ret < 0) {
        printf("Operation completed with errors\n");
        return 1;
    }
    
    printf("Operation completed successfully\n");
    return 0;
}