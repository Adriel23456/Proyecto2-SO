# TFT Display Driver

## Actualizar repositorio
cd /home/admin/Documents/Proyecto2-SO
git pull origin master

## Compilar drivers
cd /home/admin/Documents/Proyecto2-SO/DriverProgram
sudo make clean
sudo make

## Cargar drivers
sudo insmod gpio_controller.ko
sudo insmod tft_driver.ko

## Verificar carga
lsmod | grep gpio_controller
lsmod | grep tft_driver

## Probar círculo rojo
./test_tft

## Resetear pantalla
./test_tft r

## Descargar drivers
sudo rmmod tft_driver
sudo rmmod gpio_controller