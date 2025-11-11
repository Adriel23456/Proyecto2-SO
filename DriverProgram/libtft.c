/***************************************************************************//**
*  \file       libtft.c
*  \details    Implementación de la biblioteca libtft
*  \brief      Capa de abstracción entre usuario y driver del kernel
*
*  ARQUITECTURA:
*  Esta biblioteca se compila como libtft.a (biblioteca estática)
*  y es enlazada con programas de usuario. Proporciona funciones
*  que internamente:
*  1. Abren /dev/tft_device
*  2. Realizan operaciones write() o ioctl()
*  3. Manejan errores y conversiones de datos
*
*  VENTAJAS:
*  - El usuario no necesita conocer detalles del driver
*  - Maneja formato de datos (pixel_data)
*  - Provee funciones de alto nivel convenientes
*  - Maneja lectura de archivos .cvc
*******************************************************************************/
#include "libtft.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

/*
 * Ruta al dispositivo en /dev
 * Debe coincidir con el nombre creado por el driver
 */
#define TFT_DEVICE_PATH "/dev/tft_device"

/*
 * Comandos IOCTL (deben coincidir con tft_driver.h)
 */
#define TFT_IOCTL_RESET _IO('T', 0)

/*
 * Tamaño máximo del buffer para escrituras
 * Limita memoria usada y evita bloquear el kernel
 */
#define MAX_PIXELS_BUFFER 1024

/*
 * Estructura interna para píxeles (debe coincidir con el driver)
 */
struct pixel_data {
    uint16_t x;      // Coordenada X
    uint16_t y;      // Coordenada Y
    uint16_t color;  // Color RGB565
} __attribute__((packed));  // Sin padding para compatibilidad binaria

/***************************************************************************//**
* \brief Inicializa conexión con el display
*******************************************************************************/
tft_handle_t* tft_init(void)
{
    tft_handle_t *handle;
    
    // Asignar memoria para el handle
    handle = malloc(sizeof(tft_handle_t));
    if (!handle) {
        fprintf(stderr, "Failed to allocate memory for handle\n");
        return NULL;
    }
    
    // Abrir dispositivo
    // O_RDWR: lectura/escritura (aunque solo usamos escritura)
    handle->fd = open(TFT_DEVICE_PATH, O_RDWR);
    if (handle->fd < 0) {
        perror("Failed to open TFT device");
        free(handle);
        return NULL;
    }
    
    handle->is_open = 1;
    return handle;
}

/***************************************************************************//**
* \brief Cierra conexión con el display
*******************************************************************************/
int tft_close(tft_handle_t *handle)
{
    // Validar handle
    if (!handle || !handle->is_open) {
        fprintf(stderr, "Invalid handle\n");
        return -1;
    }
    
    // Cerrar file descriptor
    close(handle->fd);
    handle->is_open = 0;
    
    // Liberar memoria
    free(handle);
    return 0;
}

/***************************************************************************//**
* \brief Reinicia el display
*******************************************************************************/
int tft_reset(tft_handle_t *handle)
{
    if (!handle || !handle->is_open) {
        fprintf(stderr, "Invalid handle\n");
        return -1;
    }
    
    // Enviar comando IOCTL al driver
    if (ioctl(handle->fd, TFT_IOCTL_RESET) < 0) {
        perror("Failed to reset display");
        return -1;
    }
    
    return 0;
}

/***************************************************************************//**
* \brief Dibuja un píxel
*
* FUNCIONAMIENTO:
* 1. Crea estructura pixel_data
* 2. Llama write() para enviar al driver
* 3. El driver recibe y dibuja el píxel
*******************************************************************************/
int tft_draw_pixel(tft_handle_t *handle, uint16_t x, uint16_t y, uint16_t color)
{
    struct pixel_data pixel;
    
    if (!handle || !handle->is_open) {
        fprintf(stderr, "Invalid handle\n");
        return -1;
    }
    
    // Preparar estructura
    pixel.x = x;
    pixel.y = y;
    pixel.color = color;
    
    // Enviar al driver mediante write()
    if (write(handle->fd, &pixel, sizeof(pixel)) != sizeof(pixel)) {
        perror("Failed to write pixel");
        return -1;
    }
    
    return 0;
}

/***************************************************************************//**
* \brief Llena toda la pantalla con un color
*
* OPTIMIZACIÓN:
* En lugar de enviar 76,800 píxeles uno por uno, los agrupa en
* buffers de MAX_PIXELS_BUFFER y los envía en lotes
*******************************************************************************/
int tft_fill_screen(tft_handle_t *handle, uint16_t color)
{
    struct pixel_data *pixels;
    int total_pixels = TFT_WIDTH * TFT_HEIGHT;
    int pixels_written = 0;
    int i;
    
    if (!handle || !handle->is_open) {
        fprintf(stderr, "Invalid handle\n");
        return -1;
    }
    
    // Asignar buffer
    pixels = malloc(MAX_PIXELS_BUFFER * sizeof(struct pixel_data));
    if (!pixels) {
        fprintf(stderr, "Failed to allocate memory\n");
        return -1;
    }
    
    // Escribir en lotes
    while (pixels_written < total_pixels) {
        // Calcular cuántos píxeles enviar en este lote
        int to_write = (total_pixels - pixels_written > MAX_PIXELS_BUFFER) ? 
                       MAX_PIXELS_BUFFER : (total_pixels - pixels_written);
        
        // Llenar buffer con coordenadas y color
        for (i = 0; i < to_write; i++) {
            int pixel_num = pixels_written + i;
            pixels[i].x = pixel_num % TFT_WIDTH;   // Coordenada X
            pixels[i].y = pixel_num / TFT_WIDTH;   // Coordenada Y
            pixels[i].color = color;
        }
        
        // Enviar lote al driver
        if (write(handle->fd, pixels, to_write * sizeof(struct pixel_data)) < 0) {
            perror("Failed to write pixels");
            free(pixels);
            return -1;
        }
        
        pixels_written += to_write;
    }
    
    free(pixels);
    return 0;
}

/***************************************************************************//**
* \brief Dibuja rectángulo relleno
*
* PROCESO:
* 1. Valida límites
* 2. Crea buffer con todos los píxeles del rectángulo
* 3. Envía píxeles al driver en lotes
*******************************************************************************/
int tft_fill_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color)
{
    struct pixel_data *pixels;
    int total_pixels = width * height;
    int i, px, py;
    
    if (!handle || !handle->is_open) {
        fprintf(stderr, "Invalid handle\n");
        return -1;
    }
    
    // Validar que el rectángulo esté dentro del display
    if (x + width > TFT_WIDTH || y + height > TFT_HEIGHT) {
        fprintf(stderr, "Rectangle out of bounds\n");
        return -1;
    }
    
    // Asignar buffer para todos los píxeles del rectángulo
    pixels = malloc(total_pixels * sizeof(struct pixel_data));
    if (!pixels) {
        fprintf(stderr, "Failed to allocate memory\n");
        return -1;
    }
    
    // Llenar buffer con coordenadas de cada píxel
    i = 0;
    for (py = 0; py < height; py++) {
        for (px = 0; px < width; px++) {
            pixels[i].x = x + px;
            pixels[i].y = y + py;
            pixels[i].color = color;
            i++;
        }
    }
    
    // Enviar en lotes al driver
    int offset = 0;
    while (offset < total_pixels) {
        int to_write = (total_pixels - offset > MAX_PIXELS_BUFFER) ? 
                       MAX_PIXELS_BUFFER : (total_pixels - offset);
        
        if (write(handle->fd, &pixels[offset], to_write * sizeof(struct pixel_data)) < 0) {
            perror("Failed to write pixels");
            free(pixels);
            return -1;
        }
        
        offset += to_write;
    }
    
    free(pixels);
    return 0;
}

/***************************************************************************//**
* \brief Carga imagen desde archivo .cvc
*
* FORMATO .CVC:
* - Primera línea: header (se ignora)
* - Líneas restantes: X<TAB>Y<TAB>COLOR
*
* PROCESO:
* 1. Abre archivo
* 2. Lee línea por línea
* 3. Parsea coordenadas y color
* 4. Almacena en buffer dinámico (se expande si es necesario)
* 5. Envía todos los píxeles al driver en lotes
*******************************************************************************/
int tft_load_cvc_file(tft_handle_t *handle, const char *filename)
{
    FILE *fp;
    char line[256];
    struct pixel_data *pixels;
    int pixel_count = 0;
    int capacity = 10000;  // Capacidad inicial del buffer
    int x, y, color;
    
    if (!handle || !handle->is_open) {
        fprintf(stderr, "Invalid handle\n");
        return -1;
    }
    
    // Abrir archivo .cvc
    fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open CVC file");
        return -1;
    }
    
    // Asignar buffer inicial
    pixels = malloc(capacity * sizeof(struct pixel_data));
    if (!pixels) {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(fp);
        return -1;
    }
    
    // Saltar línea de header
    fgets(line, sizeof(line), fp);
    
    // Leer todas las líneas del archivo
    while (fgets(line, sizeof(line), fp)) {
        // Parsear línea: X<TAB>Y<TAB>COLOR
        if (sscanf(line, "%d\t%d\t%d", &x, &y, &color) == 3) {
            // Si el buffer está lleno, duplicar su tamaño
            if (pixel_count >= capacity) {
                capacity *= 2;
                struct pixel_data *new_pixels = realloc(
                    pixels, capacity * sizeof(struct pixel_data));
                if (!new_pixels) {
                    fprintf(stderr, "Failed to reallocate memory\n");
                    free(pixels);
                    fclose(fp);
                    return -1;
                }
                pixels = new_pixels;
            }
            
            // Almacenar píxel
            pixels[pixel_count].x = (uint16_t)x;
            pixels[pixel_count].y = (uint16_t)y;
            pixels[pixel_count].color = (uint16_t)color;
            pixel_count++;
        }
    }
    
    fclose(fp);
    
    printf("Loaded %d pixels from %s\n", pixel_count, filename);
    
    // Enviar todos los píxeles al driver en lotes
    int offset = 0;
    while (offset < pixel_count) {
        int to_write = (pixel_count - offset > MAX_PIXELS_BUFFER) ? 
                       MAX_PIXELS_BUFFER : (pixel_count - offset);
        
        if (write(handle->fd, &pixels[offset], to_write * sizeof(struct pixel_data)) < 0) {
            perror("Failed to write pixels");
            free(pixels);
            return -1;
        }
        
        offset += to_write;
    }
    
    free(pixels);
    return 0;
}

/***************************************************************************//**
* \brief Convierte RGB888 a RGB565
*
* CONVERSIÓN:
* RGB888: RRRRRRRR GGGGGGGG BBBBBBBB (24 bits)
* RGB565: RRRRRGGG GGGBBBBB (16 bits)
*
* - Rojo: toma los 5 bits más significativos (>>3, luego <<11)
* - Verde: toma los 6 bits más significativos (>>2, luego <<5)
* - Azul: toma los 5 bits más significativos (>>3)
*******************************************************************************/
uint16_t tft_rgb_to_color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) |   // Rojo: 5 bits altos
           ((g & 0xFC) << 3) |   // Verde: 6 bits altos
           (b >> 3);             // Azul: 5 bits altos
}