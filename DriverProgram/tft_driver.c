/***************************************************************************//**
*  \file       tft_driver.c
*  \details    TFT Display Driver with CVC file support
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

#define CMD_SWRESET   0x01
#define CMD_SLPOUT    0x11
#define CMD_DISPON    0x29
#define CMD_CASET     0x2A
#define CMD_PASET     0x2B
#define CMD_RAMWR     0x2C
#define CMD_MADCTL    0x36
#define CMD_COLMOD    0x3A

#define MAX_PIXELS_PER_WRITE 1024

dev_t dev = 0;
static struct class *dev_class;
static struct cdev tft_cdev;

static int tft_open(struct inode *inode, struct file *file);
static int tft_release(struct inode *inode, struct file *file);
static ssize_t tft_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static long tft_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = tft_open,
    .release = tft_release,
    .write = tft_write,
    .unlocked_ioctl = tft_ioctl,
};

static void write_color(uint16_t color)
{
    gpio_write_byte(color >> 8);
    gpio_write_byte(color & 0xFF);
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    gpio_write_command(CMD_CASET);
    gpio_write_byte(x0 >> 8);
    gpio_write_byte(x0 & 0xFF);
    gpio_write_byte(x1 >> 8);
    gpio_write_byte(x1 & 0xFF);

    gpio_write_command(CMD_PASET);
    gpio_write_byte(y0 >> 8);
    gpio_write_byte(y0 & 0xFF);
    gpio_write_byte(y1 >> 8);
    gpio_write_byte(y1 & 0xFF);

    gpio_write_command(CMD_RAMWR);
}

static void fill_screen(uint16_t color)
{
    uint32_t i;
    set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    for (i = 0; i < (uint32_t)LCD_WIDTH * LCD_HEIGHT; i++) {
        write_color(color);
    }
}

static void draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;
    
    set_window(x, y, x, y);
    write_color(color);
}

static void tft_init(void)
{
    gpio_reset_display();

    gpio_write_command(CMD_SWRESET);
    msleep(120);

    gpio_write_command(CMD_SLPOUT);
    msleep(120);

    gpio_write_command(CMD_COLMOD);
    gpio_write_byte(0x55);

    gpio_write_command(CMD_MADCTL);
    gpio_write_byte(0x48);

    gpio_write_command(CMD_DISPON);
    msleep(100);

    fill_screen(0x0000);

    pr_info("TFT Display initialized\n");
}

static int tft_open(struct inode *inode, struct file *file)
{
    pr_info("TFT Device opened\n");
    return 0;
}

static int tft_release(struct inode *inode, struct file *file)
{
    pr_info("TFT Device closed\n");
    return 0;
}

static ssize_t tft_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    struct pixel_data *pixels;
    size_t num_pixels;
    size_t i;
    int ret = 0;

    if (len % sizeof(struct pixel_data) != 0) {
        pr_err("Invalid data size. Must be multiple of pixel_data struct\n");
        return -EINVAL;
    }

    num_pixels = len / sizeof(struct pixel_data);
    if (num_pixels > MAX_PIXELS_PER_WRITE) {
        pr_warn("Too many pixels in one write. Processing first %d\n", MAX_PIXELS_PER_WRITE);
        num_pixels = MAX_PIXELS_PER_WRITE;
    }

    pixels = kmalloc(num_pixels * sizeof(struct pixel_data), GFP_KERNEL);
    if (!pixels) {
        pr_err("Failed to allocate memory for pixels\n");
        return -ENOMEM;
    }

    if (copy_from_user(pixels, buf, num_pixels * sizeof(struct pixel_data))) {
        pr_err("Failed to copy pixel data from user\n");
        ret = -EFAULT;
        goto cleanup;
    }

    for (i = 0; i < num_pixels; i++) {
        draw_pixel(pixels[i].x, pixels[i].y, pixels[i].color);
    }

    ret = num_pixels * sizeof(struct pixel_data);
    pr_info("Drew %zu pixels\n", num_pixels);

cleanup:
    kfree(pixels);
    return ret;
}

static long tft_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case TFT_IOCTL_RESET:
            pr_info("Reset display\n");
            tft_init();
            break;

        case TFT_IOCTL_DRAW_IMAGE:
            pr_info("Ready to receive image data via write()\n");
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

static int __init tft_driver_init(void)
{
    if (gpio_controller_init() < 0) {
        pr_err("Failed to initialize GPIO controller\n");
        return -1;
    }

    if (alloc_chrdev_region(&dev, 0, 1, "tft_device") < 0) {
        pr_err("Cannot allocate major number\n");
        gpio_controller_exit();
        return -1;
    }

    cdev_init(&tft_cdev, &fops);

    if (cdev_add(&tft_cdev, dev, 1) < 0) {
        pr_err("Cannot add device to system\n");
        goto r_class;
    }

    if (IS_ERR(dev_class = class_create("tft_class"))) {
        pr_err("Cannot create struct class\n");
        goto r_class;
    }

    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "tft_device"))) {
        pr_err("Cannot create device\n");
        goto r_device;
    }

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

static void __exit tft_driver_exit(void)
{
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&tft_cdev);
    unregister_chrdev_region(dev, 1);
    gpio_controller_exit();
    pr_info("TFT Driver removed\n");
}

module_init(tft_driver_init);
module_exit(tft_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("System Driver");
MODULE_DESCRIPTION("TFT Display Driver with CVC support");
MODULE_VERSION("2.0");