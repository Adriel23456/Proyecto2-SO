/***************************************************************************//**
*  \file       tft_driver.c
*  \details    TFT Display Driver with draw operations
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/delay.h>

// External functions from gpio_controller
extern int gpio_controller_init(void);
extern void gpio_controller_exit(void);
extern void gpio_write_command(uint8_t cmd);
extern void gpio_write_byte(uint8_t data);
extern void gpio_reset_display(void);

// Display dimensions (adjust for your display)
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// Basic ILI9341 commands
#define CMD_SWRESET   0x01
#define CMD_SLPOUT    0x11
#define CMD_DISPON    0x29
#define CMD_CASET     0x2A
#define CMD_PASET     0x2B
#define CMD_RAMWR     0x2C
#define CMD_MADCTL    0x36
#define CMD_COLMOD    0x3A

// IOCTL commands
#define TFT_IOCTL_RESET     _IO('T', 0)
#define TFT_IOCTL_DRAW_CIRCLE _IO('T', 1)

dev_t dev = 0;
static struct class *dev_class;
static struct cdev tft_cdev;

// Function prototypes
static int tft_open(struct inode *inode, struct file *file);
static int tft_release(struct inode *inode, struct file *file);
static long tft_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = tft_open,
    .release = tft_release,
    .unlocked_ioctl = tft_ioctl,
};

// Write 16-bit color
static void write_color(uint16_t color)
{
    gpio_write_byte(color >> 8);
    gpio_write_byte(color & 0xFF);
}

// Set drawing window
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

// Fill screen with color
static void fill_screen(uint16_t color)
{
    uint32_t i;
    set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    for (i = 0; i < (uint32_t)LCD_WIDTH * LCD_HEIGHT; i++) {
        write_color(color);
    }
}

// Draw filled circle
static void draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int16_t x, y;
    for (y = -r; y <= r; y++) {
        for (x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                uint16_t px = x0 + x;
                uint16_t py = y0 + y;
                if (px < LCD_WIDTH && py < LCD_HEIGHT) {
                    set_window(px, py, px, py);
                    write_color(color);
                }
            }
        }
    }
}

// Initialize TFT display
static void tft_init(void)
{
    gpio_reset_display();

    gpio_write_command(CMD_SWRESET);
    msleep(120);

    gpio_write_command(CMD_SLPOUT);
    msleep(120);

    gpio_write_command(CMD_COLMOD);
    gpio_write_byte(0x55);  // 16-bit color

    gpio_write_command(CMD_MADCTL);
    gpio_write_byte(0x48);  // RGB order

    gpio_write_command(CMD_DISPON);
    msleep(100);

    fill_screen(0x0000);  // Clear to black

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

static long tft_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case TFT_IOCTL_RESET:
            pr_info("Reset display\n");
            tft_init();
            break;

        case TFT_IOCTL_DRAW_CIRCLE:
            pr_info("Drawing red circle\n");
            fill_screen(0x0000);  // Black background
            draw_circle(LCD_WIDTH / 2, LCD_HEIGHT / 2, 50, 0xF800);  // Red circle
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
MODULE_DESCRIPTION("TFT Display Driver");
MODULE_VERSION("1.0");