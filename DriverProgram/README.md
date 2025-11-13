# Sistema de Driver TFT para Raspberry Pi

## Descripción del Proyecto

Este proyecto implementa un driver completo para pantalla TFT con interfaz paralela de 8 bits en Raspberry Pi, cumpliendo con los requisitos académicos especificados.

### 4.1 Dispositivo Físico - PANTALLA TFT

**Interfaz Seleccionada:** GPIO Paralela de 8 bits

**Justificación Técnica:**
- **Control Total:** Acceso directo al hardware sin capas de abstracción intermedias
- **Velocidad:** Comunicación paralela más rápida que alternativas serie (SPI/I2C)
- **Aprendizaje:** Permite entender protocolos de comunicación a bajo nivel
- **Flexibilidad:** Compatible con múltiples controladores de display (ILI9341, ST7735, etc.)
- **Disponibilidad:** GPIOs abundantes en Raspberry Pi permiten este tipo de interfaz

**Conexiones Físicas:**
```
Raspberry Pi GPIO → Pantalla TFT
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
GPIO 25 (537) → LCD_RS (Data/Command Select)
GPIO 23 (535) → LCD_WR (Write Enable)
GPIO 24 (536) → LCD_RST (Reset)
GPIO 5 (517)  → LCD_D0 (Data Bit 0)
GPIO 6 (518)  → LCD_D1 (Data Bit 1)
GPIO 12 (524) → LCD_D2 (Data Bit 2)
GPIO 13 (525) → LCD_D3 (Data Bit 3)
GPIO 16 (528) → LCD_D4 (Data Bit 4)
GPIO 19 (531) → LCD_D5 (Data Bit 5)
GPIO 20 (532) → LCD_D6 (Data Bit 6)
GPIO 21 (533) → LCD_D7 (Data Bit 7)

Alimentación:
3.3V → LCD_RD (Read - siempre HIGH)
GND  → LCD_CS (Chip Select - siempre activo)
```

### 4.2 Driver del Kernel

**Módulos Implementados:**

1. **gpio_controller.ko**
   - Controla directamente los GPIOs del Raspberry Pi
   - Implementa protocolo de escritura paralela
   - Exporta funciones para uso por otros módulos
   - NO usa drivers existentes del sistema

2. **tft_driver.ko**
   - Driver principal del dispositivo de caracteres
   - Crea `/dev/tft_device` para comunicación userspace
   - Implementa operaciones: open, close, write, ioctl
   - Inicializa display con secuencia específica del controlador

**Características:**
- ✅ Escrito completamente desde cero en C
- ✅ Módulo cargable del kernel Linux (lsmod)
- ✅ NO utiliza drivers comerciales ni del sistema
- ✅ Protocolo de comunicación implementado manualmente
- ✅ Documentación exhaustiva en código fuente

### 4.3 Biblioteca de Usuario (libtft.a)

**Archivo Generado:** `libtft.a` (biblioteca estática)

**Propósito:**
- Única interfaz entre programas de usuario y el driver del kernel
- Abstrae complejidad de comunicación con `/dev/tft_device`
- Proporciona API de alto nivel intuitiva

**Funciones Provistas:**

| Función | Propósito | Equivalente Hardware |
|---------|-----------|----------------------|
| `tft_init()` | Inicializar conexión | Open device |
| `tft_close()` | Cerrar conexión | Close device |
| `tft_reset()` | Resetear display | Hardware reset |
| `tft_draw_pixel()` | Dibujar píxel | **Write** (bajo nivel) |
| `tft_fill_screen()` | Llenar pantalla | **Write** (optimizado) |
| `tft_fill_rect()` | Dibujar rectángulo | **Write** (grupo) |
| `tft_load_cvc_file()` | Cargar imagen | **Read** archivo + **Write** display |
| `tft_rgb_to_color()` | Convertir color | Utilidad |

---

## Estructura del Sistema
```
┌─────────────────────────────────────────────────────┐
│                  USERSPACE                          │
│                                                     │
│  ┌──────────────┐         ┌─────────────────────┐ │
│  │  test_tft    │────────▶│    libtft.a         │ │
│  │  (programa)  │         │  (BIBLIOTECA        │ │
│  └──────────────┘         │   ESTÁTICA)         │ │
│                           │                     │ │
│                           │  • tft_init()       │ │
│                           │  • tft_fill()       │ │
│                           │  • tft_load_cvc()   │ │
│                           └────────┬────────────┘ │
└───────────────────────────────────┼──────────────┘
                                    │
                    open/write/ioctl│
                                    ▼
         ┌────────────────────────────────────────┐
         │          /dev/tft_device               │
         └────────────────┬───────────────────────┘
                          │
┌─────────────────────────┼───────────────────────┐
│          KERNEL SPACE   │                       │
│                         ▼                       │
│         ┌───────────────────────────┐           │
│         │    tft_driver.ko          │           │
│         │  (DRIVER PRINCIPAL)       │           │
│         │                           │           │
│         │  • open()                 │           │
│         │  • write()                │           │
│         │  • ioctl()                │           │
│         │  • Gestión dispositivo    │           │
│         └──────────┬────────────────┘           │
│                    │ llama funciones             │
│                    ▼                             │
│         ┌───────────────────────────┐           │
│         │  gpio_controller.ko       │           │
│         │  (DRIVER GPIO)            │           │
│         │                           │           │
│         │  • gpio_write_command()   │           │
│         │  • gpio_write_byte()      │           │
│         │  • gpio_reset_display()   │           │
│         └──────────┬────────────────┘           │
└────────────────────┼──────────────────────────┘
                     │ Control directo
                     ▼
          ┌─────────────────────────┐
          │   HARDWARE GPIO         │
          │   (Raspberry Pi pins)   │
          └────────┬────────────────┘
                   │
                   ▼
          ┌─────────────────────────┐
          │   PANTALLA TFT          │
          │   (ILI9341/compatible)  │
          └─────────────────────────┘
```

---

## Instalación y Uso

### Actualizar repositorio
```bash
cd ~/Documents/Proyecto2-SO
git pull origin master
```

### Compilar todo el sistema
```bash
cd ~/Documents/Proyecto2-SO/DriverProgram
sudo make clean
sudo make
```

**Resultado esperado:**
```
==== Compilando módulos del kernel ====
  CC [M]  gpio_controller.o
  CC [M]  tft_driver.o
  LD [M]  gpio_controller.ko
  LD [M]  tft_driver.ko
✓ Módulos compilados

==== Creando biblioteca estática ====
✓ Biblioteca compilada exitosamente

==== Compilando herramientas ====
✓ Herramientas compiladas exitosamente

COMPILACIÓN COMPLETADA EXITOSAMENTE
```

### Cargar drivers en el kernel
```bash
# Cargar driver GPIO (primero)
sudo insmod gpio_controller.ko

# Cargar driver TFT (segundo, depende de GPIO)
sudo insmod tft_driver.ko

# Verificar que se cargaron correctamente
lsmod | grep -E 'tft|gpio'

# Copiar la librería estática
sudo cp libtft.a /usr/local/lib/

# Copiar el header público
sudo cp libtft.h /usr/local/include/

#Extra para asegurar que todo bien!
sudo ldconfig
```

**Salida esperada:**
```
tft_driver      12288  0
gpio_controller 12288  1 tft_driver
```

### Verificar dispositivo creado
```bash
ls -l /dev/tft_device
```

**Salida esperada:**
```
crw------- 1 root root 240, 0 Nov 10 12:00 /dev/tft_device
```

### Dar permisos (opcional, o usar sudo)
```bash
sudo chmod 666 /dev/tft_device
```

---

## Pruebas del Sistema

### 1. Llenar pantalla con colores
```bash
# Llenar con rojo
sudo ./test_tft fill F800

# Llenar con verde
sudo ./test_tft fill 07E0

# Llenar con azul
sudo ./test_tft fill 001F

# Otros colores
sudo ./test_tft fill FFFF  # Blanco
sudo ./test_tft fill 0000  # Negro
sudo ./test_tft fill FFFF00 # Amarillo (truncado a FFE0)
```

### 2. Cargar imagen desde archivo CVC
```bash
# Generar imagen de ejemplo
./generate_histogram
cp ~/Documents/Proyecto2-SO/MainSystem/Master/result_histogram.cvc .

# Mostrar en pantalla
sudo ./test_tft cvc histogram.cvc
```

### 3. Dibujar rectángulos
```bash
# Rectángulo rojo en (50, 50) de 100x80 píxeles
sudo ./test_tft rect 50 50 100 80 F800

# Rectángulo azul en centro
sudo ./test_tft rect 70 120 100 80 001F

# Rectángulo verde en esquina
sudo ./test_tft rect 0 0 50 50 07E0
```

---

## Formato de Archivos CVC

Los archivos `.cvc` definen imágenes píxel por píxel:
```
pixelx	pixely	value
0	0	0
1	0	31
2	0	63
...
239	319	65535
```

- **pixelx:** Coordenada X (0-239)
- **pixely:** Coordenada Y (0-319)
- **value:** Color RGB565 (0-65535)

### Conversión de colores

RGB888 → RGB565:
```
R (0-255) → 5 bits (RRRRR)
G (0-255) → 6 bits (GGGGGG)
B (0-255) → 5 bits (BBBBB)

Resultado: RRRRRGGGGGGBBBBB (16 bits)
```

Ejemplo:
```c
Rojo puro (255, 0, 0)   → 0xF800 = 63488
Verde puro (0, 255, 0)  → 0x07E0 = 2016
Azul puro (0, 0, 255)   → 0x001F = 31
```

---

## Descargar Drivers
```bash
# Descargar en orden inverso a la carga
sudo rmmod tft_driver
sudo rmmod gpio_controller

# Verificar descarga
lsmod | grep -E 'tft|gpio'
# (no debería mostrar nada)
```

---

## Solución de Problemas

### Error: "Failed to open TFT device"

**Causa:** Drivers no cargados o dispositivo no existe

**Solución:**
```bash
# Verificar drivers
lsmod | grep tft_driver

# Si no están, cargarlos
sudo insmod gpio_controller.ko
sudo insmod tft_driver.ko

# Verificar dispositivo
ls -l /dev/tft_device
```

### Error: "Permission denied"

**Causa:** Sin permisos para acceder al dispositivo

**Solución:**
```bash
# Opción 1: Ejecutar con sudo
sudo ./test_tft fill F800

# Opción 2: Cambiar permisos
sudo chmod 666 /dev/tft_device
```

### Error: "Cannot allocate major number"

**Causa:** Drivers ya cargados o conflicto

**Solución:**
```bash
# Descargar drivers existentes
sudo rmmod tft_driver
sudo rmmod gpio_controller

# Recargar
sudo insmod gpio_controller.ko
sudo insmod tft_driver.ko
```

### Error de compilación: "kernel headers not found"

**Causa:** Headers del kernel no instalados

**Solución:**
```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers
```

---

## Verificación del Sistema

### Ver logs del kernel
```bash
# Ver últimos mensajes
dmesg | tail -20

# Ver solo mensajes del driver
dmesg | grep -E 'TFT|GPIO'
```

**Salida esperada:**
```
[  123.456] GPIO Controller initialized
[  123.789] Major = 240 Minor = 0
[  124.012] TFT Display initialized
[  124.123] TFT Driver loaded successfully
```

### Ver estado de módulos
```bash
lsmod | grep -E 'tft|gpio'
```

### Ver información del dispositivo
```bash
# Permisos y propietario
ls -l /dev/tft_device

# Número mayor/menor
stat /dev/tft_device
```

---

## Archivos del Proyecto
```
DriverProgram/
├── gpio_controller.c     # Driver GPIO (kernel space)
├── tft_driver.c          # Driver TFT principal (kernel space)
├── tft_driver.h          # Header compartido kernel/user
├── libtft.c              # Implementación biblioteca
├── libtft.h              # API pública biblioteca
├── test_tft.c            # Programa de prueba
├── generate_histogram.c   # Generador de ejemplos CVC
├── Makefile              # Sistema de compilación
└── README.md             # Esta documentación

Generados al compilar:
├── gpio_controller.ko    # Módulo del kernel GPIO
├── tft_driver.ko         # Módulo del kernel TFT
├── libtft.a              # Biblioteca estática
├── test_tft              # Ejecutable de prueba
├── generate_histogram    # Ejecutable generador
└── histogram.cvc         # Archivo de ejemplo
```

---

## Colores RGB565 Comunes

| Color | RGB888 | RGB565 | Hexadecimal |
|-------|--------|--------|-------------|
| Negro | (0,0,0) | 00000000000000000 | 0x0000 |
| Azul | (0,0,255) | 00000000000111111 | 0x001F |
| Verde | (0,255,0) | 00000111111000000 | 0x07E0 |
| Cyan | (0,255,255) | 00000111111111111 | 0x07FF |
| Rojo | (255,0,0) | 11111000000000000 | 0xF800 |
| Magenta | (255,0,255) | 11111000000111111 | 0xF81F |
| Amarillo | (255,255,0) | 11111111111000000 | 0xFFE0 |
| Blanco | (255,255,255) | 11111111111111111 | 0xFFFF |

---

## Licencia

GPL - GNU General Public License

Este proyecto es software libre y puede ser modificado y redistribuido bajo los términos de la GPL.

---

## Autores

Sistema de Driver TFT - Proyecto de Sistemas Operativos

---

## Notas Técnicas

### Temporización del Protocolo

El protocolo de escritura paralela requiere:
- Setup time: 1µs (datos estables antes de WR)
- Hold time: 1µs (datos estables después de WR)
- Pulse width: 2µs (duración total del ciclo)

### Rendimiento

- Píxeles por segundo: ~50,000 (depende de optimización)
- Tiempo para llenar pantalla completa: ~1.5 segundos
- Throughput: ~800 Kbps (kilobits por segundo)

### Limitaciones

- Máximo 1024 píxeles por escritura (MAX_PIXELS_PER_WRITE)
- Sin aceleración por hardware
- Sin DMA (acceso directo a memoria)
- Operaciones bloqueantes (síncronas)

### Posibles Mejoras Futuras

1. Implementar DMA para transferencias más rápidas
2. Buffer circular para escrituras asíncronas
3. Soporte para múltiples displays
4. Rotación y escalado por hardware
5. Fuentes de texto integradas