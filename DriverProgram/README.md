# TFT Display Driver System - Complete Implementation

## Descripción del Sistema

### 4.1 Dispositivo Físico
- **Interfaz:** GPIO paralelo de 8 bits
- **Justificación:** Comunicación directa de alta velocidad con el hardware, control total sobre los pines, ideal para sistemas embebidos Raspberry Pi
- **Función:** Visualización de histogramas de procesamiento de imágenes

### 4.2 Driver del Kernel
- **Lenguaje:** C
- **Plataforma:** Linux Kernel Module
- **Funcionalidad:** Interfaz de bajo nivel con hardware TFT via GPIO

### 4.3 Biblioteca de Usuario (libtft.a)
- **Archivo:** libtft.a (biblioteca estática)
- **Header:** libtft.h
- **Funciones:** Read, Write, Draw, Fill, Histogram

---

## Estructura del Proyecto
```
DriverProgram/
├── gpio_controller.c     # Driver GPIO (kernel space)
├── tft_driver.c          # Driver TFT principal (kernel space)
├── tft_driver.h          # Header compartido
├── libtft.c              # Implementación biblioteca
├── libtft.h              # API pública biblioteca
├── libtft.a              # Biblioteca compilada
├── test_tft.c            # Programa de prueba
├── generate_histogram.c   # Generador CVC
├── generate_histogram_data.c  # Generador datos
├── Makefile              # Build system
└── README.md             # Esta documentación
```

---

## Compilación

### Actualizar repositorio
```bash
cd /home/admin/Documents/Proyecto2-SO
git pull origin master
```

### Compilar todo el sistema
```bash
cd /home/admin/Documents/Proyecto2-SO/DriverProgram
sudo make clean
sudo make
```

Esto genera:
- `gpio_controller.ko` - Módulo kernel GPIO
- `tft_driver.ko` - Módulo kernel TFT
- `libtft.a` - Biblioteca estática
- `test_tft` - Programa de prueba
- `generate_histogram` - Generador de imágenes CVC
- `generate_histogram_data` - Generador de datos

---

## Carga de Drivers
```bash
# Cargar módulos del kernel
sudo insmod gpio_controller.ko
sudo insmod tft_driver.ko

# Verificar carga
lsmod | grep gpio_controller
lsmod | grep tft_driver

# Verificar dispositivo
ls -l /dev/tft_device
```

---

## Uso de la Biblioteca libtft.a

### API Disponible

#### Inicialización
```c
tft_handle_t* tft_init(void);
int tft_close(tft_handle_t *handle);
int tft_reset(tft_handle_t *handle);
```

#### Operaciones de Dibujo
```c
int tft_draw_pixel(tft_handle_t *handle, uint16_t x, uint16_t y, uint16_t color);
int tft_fill_screen(tft_handle_t *handle, uint16_t color);
int tft_clear(tft_handle_t *handle);
int tft_fill_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color);
int tft_draw_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color);
```

#### Operaciones de Alto Nivel
```c
int tft_load_cvc_file(tft_handle_t *handle, const char *filename);
int tft_draw_histogram(tft_handle_t *handle, const int *values, 
                       int num_bars, int max_value);
```

#### Utilidades
```c
uint16_t tft_rgb_to_color(uint8_t r, uint8_t g, uint8_t b);
const char* tft_get_error(void);
```

---

## Ejemplos de Uso

### 1. Reset de pantalla
```bash
sudo ./test_tft reset
```

### 2. Limpiar pantalla
```bash
sudo ./test_tft clear
```

### 3. Llenar con color
```bash
sudo ./test_tft fill F800  # Rojo (RGB565 hex)
sudo ./test_tft fill 07E0  # Verde
sudo ./test_tft fill 001F  # Azul
```

### 4. Cargar imagen CVC
```bash
./generate_histogram
sudo ./test_tft cvc histogram.cvc
```

### 5. Dibujar histograma desde datos
```bash
./generate_histogram_data 20
sudo ./test_tft histogram histogram_data.dat
```

### 6. Dibujar rectángulo
```bash
sudo ./test_tft rect 50 50 100 80 F800
```

### 7. Demostración completa
```bash
sudo ./test_tft demo
```

---

## Programación con libtft.a

### Ejemplo básico
```c
#include "libtft.h"
#include <stdio.h>

int main(void)
{
    tft_handle_t *tft;
    int histogram_data[] = {50, 80, 120, 90, 150};
    
    // Inicializar
    tft = tft_init();
    if (!tft) {
        fprintf(stderr, "Error: %s\n", tft_get_error());
        return 1;
    }
    
    // Limpiar pantalla
    tft_clear(tft);
    
    // Dibujar histograma
    tft_draw_histogram(tft, histogram_data, 5, 150);
    
    // Cerrar
    tft_close(tft);
    return 0;
}
```

### Compilar programa personalizado
```bash
gcc -o mi_programa mi_programa.c -L. -ltft -lm
sudo ./mi_programa
```

---

## Instalación de la Biblioteca (Opcional)
```bash
sudo make install_library
```

Esto instala:
- `/usr/local/lib/libtft.a`
- `/usr/local/include/libtft.h`

Después puedes compilar sin -L.:
```bash
gcc -o programa programa.c -ltft -lm
```

---

## Descarga de Drivers
```bash
sudo rmmod tft_driver
sudo rmmod gpio_controller
```

---

## Verificación del Sistema
```bash
# Ver logs del kernel
dmesg | tail -20

# Ver estado de módulos
lsmod | grep -E "tft|gpio"

# Ver permisos del dispositivo
ls -l /dev/tft_device

# Ver archivos generados
ls -lh *.ko *.a test_tft
```

---

## Solución de Problemas

### Error: Permission denied
```bash
sudo chmod 666 /dev/tft_device
```

### Error: Cannot open device
```bash
# Verificar que los módulos estén cargados
lsmod | grep tft_driver

# Recargar si es necesario
sudo rmmod tft_driver gpio_controller
sudo insmod gpio_controller.ko
sudo insmod tft_driver.ko
```

### Error de compilación
```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers
```

---

## Estructura de Archivos CVC
```
pixelx  pixely  value
0       0       0
1       0       65535
2       0       31
...
```

- **pixelx:** Coordenada X (0-239)
- **pixely:** Coordenada Y (0-319)
- **value:** Color RGB565 (0-65535)

---

## Colores Predefinidos (RGB565)
```c
TFT_COLOR_BLACK   = 0x0000
TFT_COLOR_WHITE   = 0xFFFF
TFT_COLOR_RED     = 0xF800
TFT_COLOR_GREEN   = 0x07E0
TFT_COLOR_BLUE    = 0x001F
TFT_COLOR_YELLOW  = 0xFFE0
TFT_COLOR_CYAN    = 0x07FF
TFT_COLOR_MAGENTA = 0xF81F
```