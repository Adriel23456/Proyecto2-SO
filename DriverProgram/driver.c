/***************************************************************************//**
*  \file       driver.c
*
*  \details    Simple GPIO driver - Updated for kernel 6.12
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>

#define GPIO_NUM (24+512)  // GPIO 24 on GPIO controller 1 (i.e., GPIO1_24)
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static struct gpio_desc *gpio_desc;
 
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
 
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = etx_read,
  .write          = etx_write,
  .open           = etx_open,
  .release        = etx_release,
};

static int etx_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}

static int etx_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}

static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;
  
  gpio_state = gpiod_get_value(gpio_desc);
  
  len = 1;
  if(copy_to_user(buf, &gpio_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }
  
  pr_info("Read function : GPIO_%d = %d \n", GPIO_NUM, gpio_state);
  
  return 0;
}

static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};
  
  if(copy_from_user(rec_buf, buf, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_%d Set = %c\n", GPIO_NUM, rec_buf[0]);
  
  if (rec_buf[0]=='1') {
    gpiod_set_value(gpio_desc, 1);
  } else if (rec_buf[0]=='0') {
    gpiod_set_value(gpio_desc, 0);
  } else {
    pr_err("Unknown command : Please provide either 1 or 0 \n");
  }
  
  return len;
}

static int __init etx_driver_init(void)
{
  if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    return -1;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
  cdev_init(&etx_cdev,&fops);
 
  if((cdev_add(&etx_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }
 
  if(IS_ERR(dev_class = class_create("etx_class"))){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }
 
  if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
    pr_err("Cannot create the Device \n");
    goto r_device;
  }
  
  gpio_desc = gpio_to_desc(GPIO_NUM);
  if (!gpio_desc) {
    pr_err("Failed to get GPIO %d descriptor\n", GPIO_NUM);
    goto r_device;
  }
  
  if (gpiod_direction_output(gpio_desc, 0) < 0) {
    pr_err("Failed to set GPIO %d as output\n", GPIO_NUM);
    goto r_device;
  }
  
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;
 
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&etx_cdev);
  unregister_chrdev_region(dev,1);
  
  return -1;
}

static void __exit etx_driver_exit(void)
{
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&etx_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.33");