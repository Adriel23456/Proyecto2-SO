/***************************************************************************//**
*  \file       gpio_controller.c
*  \details    GPIO controller for TFT display
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "tft_driver.h"

// GPIO definitions (BCM + 512 offset)
#define GPIO_RS    (25 + 512)
#define GPIO_WR    (23 + 512)
#define GPIO_RST   (24 + 512)
#define GPIO_D0    (5 + 512)
#define GPIO_D1    (6 + 512)
#define GPIO_D2    (12 + 512)
#define GPIO_D3    (13 + 512)
#define GPIO_D4    (16 + 512)
#define GPIO_D5    (19 + 512)
#define GPIO_D6    (20 + 512)
#define GPIO_D7    (21 + 512)

static int gpio_pins[] = {GPIO_RS, GPIO_WR, GPIO_RST, GPIO_D0, GPIO_D1, 
                          GPIO_D2, GPIO_D3, GPIO_D4, GPIO_D5, GPIO_D6, GPIO_D7};
static int num_gpios = sizeof(gpio_pins) / sizeof(gpio_pins[0]);

static void gpio_write_data(uint8_t data)
{
    gpio_set_value(GPIO_D0, (data >> 0) & 0x01);
    gpio_set_value(GPIO_D1, (data >> 1) & 0x01);
    gpio_set_value(GPIO_D2, (data >> 2) & 0x01);
    gpio_set_value(GPIO_D3, (data >> 3) & 0x01);
    gpio_set_value(GPIO_D4, (data >> 4) & 0x01);
    gpio_set_value(GPIO_D5, (data >> 5) & 0x01);
    gpio_set_value(GPIO_D6, (data >> 6) & 0x01);
    gpio_set_value(GPIO_D7, (data >> 7) & 0x01);
}

void gpio_write_command(uint8_t cmd)
{
    gpio_set_value(GPIO_RS, 0);
    gpio_write_data(cmd);
    gpio_set_value(GPIO_WR, 0);
    udelay(1);
    gpio_set_value(GPIO_WR, 1);
    udelay(1);
}

void gpio_write_byte(uint8_t data)
{
    gpio_set_value(GPIO_RS, 1);
    gpio_write_data(data);
    gpio_set_value(GPIO_WR, 0);
    udelay(1);
    gpio_set_value(GPIO_WR, 1);
    udelay(1);
}

void gpio_reset_display(void)
{
    gpio_set_value(GPIO_RST, 1);
    msleep(10);
    gpio_set_value(GPIO_RST, 0);
    msleep(50);
    gpio_set_value(GPIO_RST, 1);
    msleep(100);
}

int gpio_controller_init(void)
{
    int i, ret;
    
    for (i = 0; i < num_gpios; i++) {
        ret = gpio_request(gpio_pins[i], "tft_gpio");
        if (ret < 0) {
            pr_err("Failed to request GPIO %d\n", gpio_pins[i]);
            goto cleanup;
        }
        gpio_direction_output(gpio_pins[i], 0);
    }
    
    gpio_set_value(GPIO_WR, 1);
    gpio_set_value(GPIO_RS, 1);
    gpio_set_value(GPIO_RST, 1);
    
    pr_info("GPIO Controller initialized\n");
    return 0;

cleanup:
    while (--i >= 0) {
        gpio_free(gpio_pins[i]);
    }
    return ret;
}

void gpio_controller_exit(void)
{
    int i;
    for (i = 0; i < num_gpios; i++) {
        gpio_free(gpio_pins[i]);
    }
    pr_info("GPIO Controller removed\n");
}

EXPORT_SYMBOL(gpio_write_command);
EXPORT_SYMBOL(gpio_write_byte);
EXPORT_SYMBOL(gpio_reset_display);
EXPORT_SYMBOL(gpio_controller_init);
EXPORT_SYMBOL(gpio_controller_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("System Driver");
MODULE_DESCRIPTION("GPIO Controller for TFT Display");