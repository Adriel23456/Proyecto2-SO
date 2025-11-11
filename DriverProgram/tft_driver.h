/***************************************************************************//**
*  \file       tft_driver.h
*  \details    Header compartido para el sistema de driver TFT
*  \brief      Define estructuras, constantes e interfaces entre kernel/userspace
*
*  Este header es compartido entre:
*  - Módulos del kernel (gpio_controller.c, tft_driver.c)
*  - Biblioteca de usuario (libtft.c)
*  - Programas de usuario (test_tft.c)
*******************************************************************************/
#ifndef TFT_DRIVER_H
#define TFT_DRIVER_H

#include <linux/types.h>
#include <linux/ioctl.h>

/*
 * Dimensiones de la pantalla TFT
 * Estas son las dimensiones físicas de la pantalla conectada
 */
#define LCD_WIDTH  240   // Ancho en píxeles
#define LCD_HEIGHT 320   // Alto en píxeles

/*
 * Comandos IOCTL para comunicación userspace -> kernel
 * _IO es un macro que genera números únicos para cada comando
 * 'T' es el "magic number" identificador del dispositivo
 */
#define TFT_IOCTL_RESET      _IO('T', 0)  // Resetear y reinicializar display
#define TFT_IOCTL_DRAW_IMAGE _IO('T', 1)  // Preparar para recibir imagen

/*
 * Estructura para transferir datos de píxeles
 * Empaquetada para evitar padding y garantizar compatibilidad binaria
 * entre userspace y kernel space
 */
struct pixel_data {
    uint16_t x;      // Coordenada X (0-239)
    uint16_t y;      // Coordenada Y (0-319)
    uint16_t color;  // Color RGB565 (16 bits)
} __attribute__((packed));

/*
 * Prototipos de funciones exportadas por gpio_controller
 * Estas funciones son llamadas por tft_driver para controlar el hardware
 */
int gpio_controller_init(void);           // Inicializar todos los GPIOs
void gpio_controller_exit(void);          // Liberar todos los GPIOs
void gpio_write_command(uint8_t cmd);     // Enviar comando al display
void gpio_write_byte(uint8_t data);       // Enviar dato al display
void gpio_reset_display(void);            // Reset hardware del display

#endif /* TFT_DRIVER_H */