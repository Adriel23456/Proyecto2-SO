# TFT Display Driver - CVC Image System

## Actualizar repositorio
cd /home/admin/Documents/Proyecto2-SO
git pull origin master

## Compilar drivers y herramientas
cd /home/admin/Documents/Proyecto2-SO/DriverProgram
sudo make clean
sudo make

## Cargar drivers
sudo insmod gpio_controller.ko
sudo insmod tft_driver.ko

## Verificar carga
lsmod | grep gpio_controller
lsmod | grep tft_driver

## Generar imagen de ejemplo
./generate_histogram

## Mostrar imagen en pantalla
sudo ./test_tft histogram.cvc

## Resetear pantalla
sudo ./test_tft reset

## Descargar drivers
sudo rmmod tft_driver
sudo rmmod gpio_controller