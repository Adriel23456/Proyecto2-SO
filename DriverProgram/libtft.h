/***************************************************************************//**
*  \file       libtft.h
*  \details    API pública de la biblioteca libtft
*  \brief      Interfaz de usuario para controlar el display TFT
*
*  PROPÓSITO DE LA BIBLIOTECA:
*  Esta biblioteca (libtft.a) abstrae la comunicación con el driver del kernel.
*  Proporciona funciones de alto nivel para:
*  - Inicializar/cerrar conexión con el display
*  - Dibujar píxeles, rectángulos, llenar pantalla
*  - Cargar imágenes desde archivos .cvc
*
*  FLUJO:
*  Programa usuario -> libtft.a -> /dev/tft_device -> tft_driver.ko -> gpio_controller.ko -> Hardware
*******************************************************************************/
#ifndef LIBTFT_H
#define LIBTFT_H

#include <stdint.h>

/*
 * Dimensiones del display (deben coincidir con el driver)
 */
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

/*
 * Handle opaco para la biblioteca
 * Contiene el file descriptor del dispositivo y estado
 */
typedef struct {
    int fd;        // File descriptor de /dev/tft_device
    int is_open;   // Bandera: 1 si está abierto, 0 si cerrado
} tft_handle_t;

/***************************************************************************//**
* \brief Inicializa conexión con el display TFT
* \return Puntero al handle si éxito, NULL si error
*
* FUNCIONAMIENTO:
* - Abre /dev/tft_device
* - Asigna estructura handle
* - Retorna handle para usar en otras funciones
*
* EJEMPLO DE USO:
*   tft_handle_t *tft = tft_init();
*   if (!tft) {
*       fprintf(stderr, "Error al inicializar\n");
*       return 1;
*   }
*******************************************************************************/
tft_handle_t* tft_init(void);

/***************************************************************************//**
* \brief Cierra conexión con el display
* \param handle Handle obtenido de tft_init()
* \return 0 si éxito, -1 si error
*
* FUNCIONAMIENTO:
* - Cierra file descriptor
* - Libera memoria del handle
*
* SIEMPRE llamar al finalizar el programa
*******************************************************************************/
int tft_close(tft_handle_t *handle);

/***************************************************************************//**
* \brief Reinicia el display a estado inicial
* \param handle Handle del display
* \return 0 si éxito, -1 si error
*
* Envía comando IOCTL de reset al driver
*******************************************************************************/
int tft_reset(tft_handle_t *handle);

/***************************************************************************//**
* \brief Dibuja un píxel individual
* \param handle Handle del display
* \param x Coordenada X (0-239)
* \param y Coordenada Y (0-319)
* \param color Color en formato RGB565
* \return 0 si éxito, -1 si error
*
* NOTA: Dibujar píxeles uno por uno es LENTO
* Para muchos píxeles, usar tft_fill_rect() o tft_load_cvc_file()
*******************************************************************************/
int tft_draw_pixel(tft_handle_t *handle, uint16_t x, uint16_t y, uint16_t color);

/***************************************************************************//**
* \brief Llena toda la pantalla con un color
* \param handle Handle del display
* \param color Color RGB565
* \return 0 si éxito, -1 si error
*
* Escribe 76,800 píxeles (240x320)
* Toma aproximadamente 1-2 segundos
*******************************************************************************/
int tft_fill_screen(tft_handle_t *handle, uint16_t color);

/***************************************************************************//**
* \brief Dibuja rectángulo relleno
* \param handle Handle del display
* \param x Coordenada X esquina superior izquierda
* \param y Coordenada Y esquina superior izquierda
* \param width Ancho del rectángulo
* \param height Alto del rectángulo
* \param color Color RGB565
* \return 0 si éxito, -1 si error
*
* Valida que el rectángulo esté dentro de los límites del display
*******************************************************************************/
int tft_fill_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color);

/***************************************************************************//**
* \brief Carga y dibuja imagen desde archivo .cvc
* \param handle Handle del display
* \param filename Ruta al archivo .cvc
* \return 0 si éxito, -1 si error
*
* FORMATO .CVC:
* Primera línea: pixelx<TAB>pixely<TAB>value
* Líneas siguientes: X<TAB>Y<TAB>COLOR
*
* EJEMPLO:
*   pixelx  pixely  value
*   0       0       0
*   1       0       65535
*   ...
*
* COLOR está en formato RGB565 (0-65535)
*******************************************************************************/
int tft_load_cvc_file(tft_handle_t *handle, const char *filename);

/***************************************************************************//**
* \brief Convierte RGB (8 bits por canal) a RGB565
* \param r Componente rojo (0-255)
* \param g Componente verde (0-255)
* \param b Componente azul (0-255)
* \return Color en formato RGB565
*
* RGB565: RRRRRGGGGGGBBBBB
* - 5 bits para rojo
* - 6 bits para verde (ojo humano más sensible)
* - 5 bits para azul
*
* EJEMPLO:
*   uint16_t red = tft_rgb_to_color(255, 0, 0);     // 0xF800
*   uint16_t green = tft_rgb_to_color(0, 255, 0);   // 0x07E0
*   uint16_t blue = tft_rgb_to_color(0, 0, 255);    // 0x001F
*******************************************************************************/
uint16_t tft_rgb_to_color(uint8_t r, uint8_t g, uint8_t b);

#endif /* LIBTFT_H */