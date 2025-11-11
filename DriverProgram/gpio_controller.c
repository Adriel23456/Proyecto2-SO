/***************************************************************************//**
*  \file       gpio_controller.c
*  \details    Controlador GPIO para pantalla TFT con interfaz paralela de 8 bits
*  \brief      Maneja la comunicación de bajo nivel con el hardware TFT
*
*  ARQUITECTURA:
*  Este módulo es responsable de:
*  1. Inicializar y configurar los pines GPIO del Raspberry Pi
*  2. Implementar el protocolo de comunicación paralela de 8 bits
*  3. Proveer funciones de alto nivel para escribir comandos y datos
*
*  CONEXIONES FÍSICAS:
*  - GPIO 25 (RS/DC): Selecciona entre comando (0) y dato (1)
*  - GPIO 23 (WR): Write strobe (flanco de bajada escribe dato)
*  - GPIO 24 (RST): Reset hardware
*  - GPIO 5-7, 12-13, 16, 19-21: Bus de datos paralelo de 8 bits (D0-D7)
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "tft_driver.h"

/*
 * DEFINICIONES DE PINES GPIO
 * Nota: Raspberry Pi usa un offset de 512 para el controlador GPIO
 * Entonces GPIO físico 24 = GPIO 536 en el sistema (24 + 512)
 */
#define GPIO_RS    (25 + 512)  // Register Select: 0=comando, 1=dato
#define GPIO_WR    (23 + 512)  // Write Enable: pulso bajo escribe
#define GPIO_RST   (24 + 512)  // Reset: pulso bajo resetea display
#define GPIO_D0    (5 + 512)   // Data bit 0 (LSB)
#define GPIO_D1    (6 + 512)   // Data bit 1
#define GPIO_D2    (12 + 512)  // Data bit 2
#define GPIO_D3    (13 + 512)  // Data bit 3
#define GPIO_D4    (16 + 512)  // Data bit 4
#define GPIO_D5    (19 + 512)  // Data bit 5
#define GPIO_D6    (20 + 512)  // Data bit 6
#define GPIO_D7    (21 + 512)  // Data bit 7 (MSB)

/*
 * Array de todos los GPIOs usados
 * Facilita inicialización y limpieza en bucles
 */
static int gpio_pins[] = {
    GPIO_RS, GPIO_WR, GPIO_RST, 
    GPIO_D0, GPIO_D1, GPIO_D2, GPIO_D3, 
    GPIO_D4, GPIO_D5, GPIO_D6, GPIO_D7
};
static int num_gpios = sizeof(gpio_pins) / sizeof(gpio_pins[0]);

/***************************************************************************//**
* \brief Escribe un byte al bus de datos paralelo
* \param data Byte a escribir (8 bits)
*
* FUNCIONAMIENTO:
* - Cada bit del byte se extrae y se escribe al GPIO correspondiente
* - D0 = bit 0 (LSB), D7 = bit 7 (MSB)
* - Esta función NO genera el pulso WR, solo prepara los datos
*******************************************************************************/
static void gpio_write_data(uint8_t data)
{
    // Extraer cada bit y escribirlo al GPIO correspondiente
    gpio_set_value(GPIO_D0, (data >> 0) & 0x01);  // Bit 0
    gpio_set_value(GPIO_D1, (data >> 1) & 0x01);  // Bit 1
    gpio_set_value(GPIO_D2, (data >> 2) & 0x01);  // Bit 2
    gpio_set_value(GPIO_D3, (data >> 3) & 0x01);  // Bit 3
    gpio_set_value(GPIO_D4, (data >> 4) & 0x01);  // Bit 4
    gpio_set_value(GPIO_D5, (data >> 5) & 0x01);  // Bit 5
    gpio_set_value(GPIO_D6, (data >> 6) & 0x01);  // Bit 6
    gpio_set_value(GPIO_D7, (data >> 7) & 0x01);  // Bit 7 (MSB)
}

/***************************************************************************//**
* \brief Envía un comando al display TFT
* \param cmd Código de comando (depende del controlador del display)
*
* PROTOCOLO:
* 1. RS = 0 (modo comando)
* 2. Poner datos en el bus
* 3. WR = 0 (inicio de escritura)
* 4. Esperar 1µs (setup time)
* 5. WR = 1 (fin de escritura, display lee en flanco)
* 6. Esperar 1µs (hold time)
*******************************************************************************/
void gpio_write_command(uint8_t cmd)
{
    gpio_set_value(GPIO_RS, 0);      // Modo comando
    gpio_write_data(cmd);            // Poner comando en bus
    gpio_set_value(GPIO_WR, 0);      // Iniciar escritura
    udelay(1);                       // Tiempo de setup
    gpio_set_value(GPIO_WR, 1);      // Finalizar escritura
    udelay(1);                       // Tiempo de hold
}

/***************************************************************************//**
* \brief Envía un byte de datos al display TFT
* \param data Byte de datos a enviar
*
* PROTOCOLO:
* Idéntico a gpio_write_command pero con RS = 1 (modo dato)
* Esto indica al display que debe interpretar el byte como dato, no comando
*******************************************************************************/
void gpio_write_byte(uint8_t data)
{
    gpio_set_value(GPIO_RS, 1);      // Modo dato
    gpio_write_data(data);           // Poner dato en bus
    gpio_set_value(GPIO_WR, 0);      // Iniciar escritura
    udelay(1);                       // Tiempo de setup
    gpio_set_value(GPIO_WR, 1);      // Finalizar escritura
    udelay(1);                       // Tiempo de hold
}

/***************************************************************************//**
* \brief Realiza reset hardware del display
*
* SECUENCIA DE RESET:
* 1. RST = 1 por 10ms (estado inactivo)
* 2. RST = 0 por 50ms (activa reset)
* 3. RST = 1 por 100ms (desactiva reset, display se inicializa)
*
* Los tiempos son críticos y dependen del datasheet del controlador
*******************************************************************************/
void gpio_reset_display(void)
{
    gpio_set_value(GPIO_RST, 1);     // RST inactivo
    msleep(10);                      // Esperar estabilización
    gpio_set_value(GPIO_RST, 0);     // Activar reset
    msleep(50);                      // Mantener en reset
    gpio_set_value(GPIO_RST, 1);     // Desactivar reset
    msleep(100);                     // Esperar inicialización interna
}

/***************************************************************************//**
* \brief Inicializa todos los GPIOs necesarios para el display
* \return 0 si éxito, negativo si error
*
* PROCESO:
* 1. Solicitar cada GPIO al kernel (gpio_request)
* 2. Configurar cada GPIO como salida (gpio_direction_output)
* 3. Establecer valores iniciales seguros
*
* Si algún GPIO falla, libera todos los anteriores (cleanup)
*******************************************************************************/
int gpio_controller_init(void)
{
    int i, ret;
    
    // Solicitar cada GPIO al kernel
    for (i = 0; i < num_gpios; i++) {
        ret = gpio_request(gpio_pins[i], "tft_gpio");
        if (ret < 0) {
            pr_err("Failed to request GPIO %d\n", gpio_pins[i]);
            goto cleanup;  // Falló, liberar GPIOs ya solicitados
        }
        // Configurar como salida con valor inicial 0
        gpio_direction_output(gpio_pins[i], 0);
    }
    
    /*
     * Establecer estados iniciales seguros:
     * - WR = 1 (inactivo, no está escribiendo)
     * - RS = 1 (modo dato por defecto)
     * - RST = 1 (no en reset)
     */
    gpio_set_value(GPIO_WR, 1);
    gpio_set_value(GPIO_RS, 1);
    gpio_set_value(GPIO_RST, 1);
    
    pr_info("GPIO Controller initialized\n");
    return 0;

cleanup:
    // Si falló, liberar todos los GPIOs solicitados hasta ahora
    while (--i >= 0) {
        gpio_free(gpio_pins[i]);
    }
    return ret;
}

/***************************************************************************//**
* \brief Libera todos los GPIOs y limpia recursos
*
* Debe ser llamado al descargar el módulo
*******************************************************************************/
void gpio_controller_exit(void)
{
    int i;
    // Liberar cada GPIO devuelto al kernel
    for (i = 0; i < num_gpios; i++) {
        gpio_free(gpio_pins[i]);
    }
    pr_info("GPIO Controller removed\n");
}

/*
 * EXPORTAR SÍMBOLOS
 * Permite que tft_driver.ko use estas funciones
 * Sin EXPORT_SYMBOL, otro módulo no puede llamar estas funciones
 */
EXPORT_SYMBOL(gpio_write_command);
EXPORT_SYMBOL(gpio_write_byte);
EXPORT_SYMBOL(gpio_reset_display);
EXPORT_SYMBOL(gpio_controller_init);
EXPORT_SYMBOL(gpio_controller_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("System Driver");
MODULE_DESCRIPTION("GPIO Controller for TFT Display");