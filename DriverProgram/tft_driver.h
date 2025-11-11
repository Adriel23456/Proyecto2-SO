/***************************************************************************//**
*  \file       tft_driver.h
*  \details    Shared header for TFT driver system
*******************************************************************************/
#ifndef TFT_DRIVER_H
#define TFT_DRIVER_H

#include <linux/types.h>
#include <linux/ioctl.h>

// Display dimensions
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// IOCTL commands
#define TFT_IOCTL_RESET     _IO('T', 0)
#define TFT_IOCTL_DRAW_IMAGE _IO('T', 1)

// Pixel data structure
struct pixel_data {
    uint16_t x;
    uint16_t y;
    uint16_t color;
} __attribute__((packed));

// GPIO controller functions
int gpio_controller_init(void);
void gpio_controller_exit(void);
void gpio_write_command(uint8_t cmd);
void gpio_write_byte(uint8_t data);
void gpio_reset_display(void);

#endif /* TFT_DRIVER_H */