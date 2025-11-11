/***************************************************************************//**
*  \file       tft_driver.c
*  \details    Driver principal para pantalla TFT con soporte de archivos CVC
*  \brief      Crea /dev/tft_device y maneja operaciones de dibujo
*
*  ARQUITECTURA DEL DRIVER:
*  - Crea un dispositivo de caracteres en /dev/tft_device
*  - Implementa operaciones: open, close, write, ioctl
*  - Usa gpio_controller para la comunicación física
*  - Inicializa el display con secuencia específica del controlador ILI9341
*
*  FLUJO DE DATOS:
*  Userspace (libtft.a) -> write() -> kernel -> este driver -> gpio_controller -> Hardware
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "tft_driver.h"

/*
 * COMANDOS DEL CONTROLADOR ILI9341
 * Estos códigos son específicos del chip controlador del display
 * Referencia: ILI9341 datasheet
 */
#define CMD_SWRESET   0x01  // Software reset
#define CMD_SLPOUT    0x11  // Sleep out (despertar display)
#define CMD_DISPON    0x29  // Display on (encender display)
#define CMD_CASET     0x2A  // Column address set
#define CMD_PASET     0x2B  // Page (row) address set
#define CMD_RAMWR     0x2C  // Memory write (escribir píxeles)
#define CMD_MADCTL    0x36  // Memory access control (orientación)
#define CMD_COLMOD    0x3A  // Pixel format (profundidad de color)

/*
 * Límite de píxeles por escritura para evitar bloquear el kernel
 * Si se envían muchos píxeles a la vez, puede causar latencia
 */
#define MAX_PIXELS_PER_WRITE 1024

/*
 * Variables globales del driver
 * Administran el dispositivo de caracteres
 */
dev_t dev = 0;                    // Número mayor/menor del dispositivo
static struct class *dev_class;   // Clase del dispositivo en /sys
static struct cdev tft_cdev;      // Estructura del dispositivo de caracteres

/*
 * Prototipos de funciones de operaciones del dispositivo
 */
static int tft_open(struct inode *inode, struct file *file);
static int tft_release(struct inode *inode, struct file *file);
static ssize_t tft_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static long tft_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/*
 * Estructura de operaciones del archivo
 * Define qué funciones se llaman cuando userspace hace:
 * open(), close(), write(), ioctl() en /dev/tft_device
 */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = tft_open,
    .release = tft_release,
    .write = tft_write,
    .unlocked_ioctl = tft_ioctl,
};

/***************************************************************************//**
* \brief Escribe un color de 16 bits al display
* \param color Color en formato RGB565
*
* RGB565: RRRRRGGGGGGBBBBB (5 bits rojo, 6 verde, 5 azul)
* Se envía en 2 bytes: primero byte alto, luego byte bajo
*******************************************************************************/
static void write_color(uint16_t color)
{
    gpio_write_byte(color >> 8);    // Byte alto (RRRRRGGG)
    gpio_write_byte(color & 0xFF);  // Byte bajo (GGGBBBBB)
}

/***************************************************************************//**
* \brief Define ventana de dibujo en el display
* \param x0 Coordenada X inicial
* \param y0 Coordenada Y inicial
* \param x1 Coordenada X final
* \param y1 Coordenada Y final
*
* FUNCIONAMIENTO:
* Después de llamar esta función, cualquier escritura con RAMWR
* dibujará píxeles dentro de esta ventana, auto-incrementando posición
*
* SECUENCIA:
* 1. CASET: define columnas (x0 a x1)
* 2. PASET: define filas (y0 a y1)
* 3. RAMWR: prepara para escribir píxeles
*******************************************************************************/
static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Establecer rango de columnas
    gpio_write_command(CMD_CASET);
    gpio_write_byte(x0 >> 8);        // X inicial (byte alto)
    gpio_write_byte(x0 & 0xFF);      // X inicial (byte bajo)
    gpio_write_byte(x1 >> 8);        // X final (byte alto)
    gpio_write_byte(x1 & 0xFF);      // X final (byte bajo)

    // Establecer rango de filas
    gpio_write_command(CMD_PASET);
    gpio_write_byte(y0 >> 8);        // Y inicial (byte alto)
    gpio_write_byte(y0 & 0xFF);      // Y inicial (byte bajo)
    gpio_write_byte(y1 >> 8);        // Y final (byte alto)
    gpio_write_byte(y1 & 0xFF);      // Y final (byte bajo)

    // Preparar para escribir en memoria RAM del display
    gpio_write_command(CMD_RAMWR);
}

/***************************************************************************//**
* \brief Llena toda la pantalla con un color
* \param color Color RGB565
*
* Escribe 240x320 = 76,800 píxeles
* Toma aproximadamente 1-2 segundos completar
*******************************************************************************/
static void fill_screen(uint16_t color)
{
    uint32_t i;
    // Ventana = toda la pantalla
    set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    // Escribir color a cada píxel
    for (i = 0; i < (uint32_t)LCD_WIDTH * LCD_HEIGHT; i++) {
        write_color(color);
    }
}

/***************************************************************************//**
* \brief Dibuja un solo píxel en coordenadas específicas
* \param x Coordenada X (0-239)
* \param y Coordenada Y (0-319)
* \param color Color RGB565
*
* Valida coordenadas antes de dibujar
*******************************************************************************/
static void draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    // Validar límites
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;
    
    // Ventana de 1x1 píxel
    set_window(x, y, x, y);
    write_color(color);
}

/***************************************************************************//**
* \brief Inicializa el display TFT
*
* SECUENCIA DE INICIALIZACIÓN (específica para ILI9341):
* 1. Reset hardware
* 2. Software reset (SWRESET)
* 3. Salir de modo sleep (SLPOUT)
* 4. Configurar formato de píxel a 16 bits (COLMOD)
* 5. Configurar orientación y orden RGB (MADCTL)
* 6. Encender display (DISPON)
* 7. Limpiar pantalla a negro
*
* Los tiempos de espera son críticos según datasheet
*******************************************************************************/
static void tft_init(void)
{
    // Reset hardware
    gpio_reset_display();

    // Software reset
    gpio_write_command(CMD_SWRESET);
    msleep(120);  // Esperar reset completo

    // Despertar del modo sleep
    gpio_write_command(CMD_SLPOUT);
    msleep(120);  // Esperar estabilización

    // Configurar formato de color a RGB565 (16 bits por píxel)
    gpio_write_command(CMD_COLMOD);
    gpio_write_byte(0x55);  // 0x55 = 16 bits/píxel

    // Configurar control de memoria
    // 0x48 = orientación normal, orden RGB
    gpio_write_command(CMD_MADCTL);
    gpio_write_byte(0x48);

    // Encender display
    gpio_write_command(CMD_DISPON);
    msleep(100);  // Esperar encendido completo

    // Limpiar pantalla a negro
    fill_screen(0x0000);

    pr_info("TFT Display initialized\n");
}

/***************************************************************************//**
* \brief Callback cuando userspace abre /dev/tft_device
* \return 0 si éxito
*
* Se llama cuando: fd = open("/dev/tft_device", ...)
*******************************************************************************/
static int tft_open(struct inode *inode, struct file *file)
{
    pr_info("TFT Device opened\n");
    return 0;
}

/***************************************************************************//**
* \brief Callback cuando userspace cierra el dispositivo
* \return 0 si éxito
*
* Se llama cuando: close(fd)
*******************************************************************************/
static int tft_release(struct inode *inode, struct file *file)
{
    pr_info("TFT Device closed\n");
    return 0;
}

/***************************************************************************//**
* \brief Callback cuando userspace escribe al dispositivo
* \param buf Buffer de userspace con datos de píxeles
* \param len Longitud del buffer en bytes
* \return Número de bytes escritos o código de error negativo
*
* FUNCIONAMIENTO:
* 1. Valida que len sea múltiplo de sizeof(pixel_data)
* 2. Copia datos de userspace a kernel space
* 3. Dibuja cada píxel usando draw_pixel()
* 4. Limita píxeles por llamada para no bloquear kernel
*
* Se llama cuando: write(fd, pixels, size)
*******************************************************************************/
static ssize_t tft_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    struct pixel_data *pixels;
    size_t num_pixels;
    size_t i;
    int ret = 0;

    // Validar que el tamaño sea múltiplo de la estructura pixel_data
    if (len % sizeof(struct pixel_data) != 0) {
        pr_err("Invalid data size. Must be multiple of pixel_data struct\n");
        return -EINVAL;
    }

    // Calcular número de píxeles
    num_pixels = len / sizeof(struct pixel_data);
    
    // Limitar píxeles por escritura para evitar latencia
    if (num_pixels > MAX_PIXELS_PER_WRITE) {
        pr_warn("Too many pixels, limiting to %d\n", MAX_PIXELS_PER_WRITE);
        num_pixels = MAX_PIXELS_PER_WRITE;
    }

    // Asignar memoria en kernel space
    pixels = kmalloc(num_pixels * sizeof(struct pixel_data), GFP_KERNEL);
    if (!pixels) {
        pr_err("Failed to allocate memory for pixels\n");
        return -ENOMEM;
    }

    // Copiar datos desde userspace a kernel space
    if (copy_from_user(pixels, buf, num_pixels * sizeof(struct pixel_data))) {
        pr_err("Failed to copy pixel data from user\n");
        ret = -EFAULT;
        goto cleanup;
    }

    // Dibujar cada píxel
    for (i = 0; i < num_pixels; i++) {
        draw_pixel(pixels[i].x, pixels[i].y, pixels[i].color);
    }

    // Retornar bytes procesados
    ret = num_pixels * sizeof(struct pixel_data);

cleanup:
    kfree(pixels);
    return ret;
}

/***************************************************************************//**
* \brief Callback para comandos ioctl desde userspace
* \param cmd Comando ioctl
* \param arg Argumento del comando (no usado actualmente)
* \return 0 si éxito, negativo si error
*
* COMANDOS SOPORTADOS:
* - TFT_IOCTL_RESET: Reinicializa el display
* - TFT_IOCTL_DRAW_IMAGE: Placeholder para preparar recepción de imagen
*
* Se llama cuando: ioctl(fd, TFT_IOCTL_RESET, 0)
*******************************************************************************/
static long tft_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case TFT_IOCTL_RESET:
            pr_info("Reset display\n");
            tft_init();  // Re-inicializar display
            break;

        case TFT_IOCTL_DRAW_IMAGE:
            pr_info("Ready to receive image data\n");
            // No hace nada, pero podría preparar buffer o estado
            break;

        default:
            return -EINVAL;  // Comando no reconocido
    }
    return 0;
}

/***************************************************************************//**
* \brief Función de inicialización del módulo
* \return 0 si éxito, negativo si error
*
* PROCESO DE INICIALIZACIÓN:
* 1. Inicializar GPIO controller
* 2. Registrar dispositivo de caracteres (obtener major/minor)
* 3. Crear cdev y agregarlo al sistema
* 4. Crear clase del dispositivo en sysfs
* 5. Crear nodo del dispositivo en /dev/tft_device
* 6. Inicializar hardware del display
*
* Si algo falla, hace cleanup y retorna error
*******************************************************************************/
static int __init tft_driver_init(void)
{
    // Inicializar controlador GPIO (hardware)
    if (gpio_controller_init() < 0) {
        pr_err("Failed to initialize GPIO controller\n");
        return -1;
    }

    // Registrar dispositivo de caracteres
    // Asigna automáticamente major number
    if (alloc_chrdev_region(&dev, 0, 1, "tft_device") < 0) {
        pr_err("Cannot allocate major number\n");
        gpio_controller_exit();
        return -1;
    }
    pr_info("Major = %d Minor = %d\n", MAJOR(dev), MINOR(dev));

    // Inicializar estructura cdev con file_operations
    cdev_init(&tft_cdev, &fops);

    // Agregar dispositivo al sistema
    if (cdev_add(&tft_cdev, dev, 1) < 0) {
        pr_err("Cannot add device to system\n");
        goto r_class;
    }

    // Crear clase del dispositivo (/sys/class/tft_class)
    if (IS_ERR(dev_class = class_create("tft_class"))) {
        pr_err("Cannot create struct class\n");
        goto r_class;
    }

    // Crear nodo del dispositivo (/dev/tft_device)
    // Automáticamente crea el archivo en /dev
    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "tft_device"))) {
        pr_err("Cannot create device\n");
        goto r_device;
    }

    // Inicializar hardware del display
    tft_init();

    pr_info("TFT Driver loaded successfully\n");
    return 0;

r_device:
    class_destroy(dev_class);
r_class:
    unregister_chrdev_region(dev, 1);
    gpio_controller_exit();
    return -1;
}

/***************************************************************************//**
* \brief Función de limpieza del módulo
*
* PROCESO DE LIMPIEZA:
* 1. Destruir dispositivo en /dev
* 2. Destruir clase en /sys
* 3. Eliminar cdev del sistema
* 4. Liberar número de dispositivo
* 5. Limpiar GPIO controller
*
* Llamado cuando: sudo rmmod tft_driver
*******************************************************************************/
static void __exit tft_driver_exit(void)
{
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&tft_cdev);
    unregister_chrdev_region(dev, 1);
    gpio_controller_exit();
    pr_info("TFT Driver removed\n");
}

// Registrar funciones de init y exit
module_init(tft_driver_init);
module_exit(tft_driver_exit);

// Información del módulo
MODULE_LICENSE("GPL");
MODULE_AUTHOR("System Driver");
MODULE_DESCRIPTION("TFT Display Driver with CVC support");
MODULE_VERSION("2.0");