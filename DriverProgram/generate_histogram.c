/***************************************************************************//**
*  \file       generate_histogram.c
*  \details    Generador de archivos CVC con histograma colorido
*  \brief      Crea histogram.cvc con barras de colores aleatorios
*
*  PROPÓSITO:
*  - Genera archivo .cvc de demostración
*  - Crea histograma visual con barras de colores arcoíris
*  - Incluye fondo oscuro y líneas de cuadrícula
*
*  FORMATO DE SALIDA:
*  Primera línea: pixelx	pixely	value
*  Líneas siguientes: X	Y	COLOR (en RGB565)
*
*  EJECUCIÓN:
*    ./generate_histogram
*    # Genera histogram.cvc en el directorio actual
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

// Dimensiones del display (deben coincidir con el driver)
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

/***************************************************************************//**
* \brief Convierte RGB888 a RGB565
*
* CONVERSIÓN:
* RGB888 (8 bits por canal) -> RGB565 (5-6-5 bits)
*******************************************************************************/
uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) |   // Rojo: 5 bits superiores
           ((g & 0xFC) << 3) |   // Verde: 6 bits superiores
           (b >> 3);             // Azul: 5 bits superiores
}

/***************************************************************************//**
* \brief Convierte HSV a RGB
* \param h Hue (matiz): 0-360 grados
* \param s Saturation (saturación): 0.0-1.0
* \param v Value (valor/brillo): 0.0-1.0
* \param r Puntero para retornar componente rojo (0-255)
* \param g Puntero para retornar componente verde (0-255)
* \param b Puntero para retornar componente azul (0-255)
*
* MODELO HSV:
* Permite generar colores del arcoíris variando solo H (hue)
* mientras se mantienen S y V constantes
*******************************************************************************/
void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c = v * s;                                    // Chroma
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));  // Intermediate value
    float m = v - c;                                    // Match value
    float r1, g1, b1;

    // Determinar componentes RGB' según sector del círculo cromático
    if (h < 60) {
        r1 = c; g1 = x; b1 = 0;
    } else if (h < 120) {
        r1 = x; g1 = c; b1 = 0;
    } else if (h < 180) {
        r1 = 0; g1 = c; b1 = x;
    } else if (h < 240) {
        r1 = 0; g1 = x; b1 = c;
    } else if (h < 300) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }

    // Convertir a 0-255 agregando match value
    *r = (uint8_t)((r1 + m) * 255);
    *g = (uint8_t)((g1 + m) * 255);
    *b = (uint8_t)((b1 + m) * 255);
}

/***************************************************************************//**
* \brief Función principal - genera histogram.cvc
*******************************************************************************/
int main(void)
{
    FILE *fp;
    int x, y;
    int num_bars = 20;                           // Número de barras en histograma
    int bar_width = LCD_WIDTH / num_bars;        // Ancho de cada barra
    int bar_heights[20];                         // Altura de cada barra
    uint16_t bar_colors[20];                     // Color de cada barra

    // Crear archivo de salida
    fp = fopen("histogram.cvc", "w");
    if (!fp) {
        perror("Failed to create histogram.cvc");
        return 1;
    }

    // Escribir línea de header
    fprintf(fp, "pixelx\tpixely\tvalue\n");

    /*
     * GENERAR DATOS DEL HISTOGRAMA
     * - Alturas aleatorias pero reproducibles (seed fijo)
     * - Colores del arcoíris (HSV con H variando)
     */
    srand(42);  // Seed fijo para resultados reproducibles
    
    for (int i = 0; i < num_bars; i++) {
        // Altura aleatoria entre 50 y 250 píxeles
        bar_heights[i] = 50 + rand() % 200;
        
        // Color del arcoíris: variar hue de 0° a 360°
        uint8_t r, g, b;
        float hue = (360.0 * i) / num_bars;  // Distribuir uniformemente en círculo
        hsv_to_rgb(hue, 0.9, 0.9, &r, &g, &b);  // Alta saturación y brillo
        bar_colors[i] = rgb_to_rgb565(r, g, b);
    }

    printf("Generating colorful histogram with %d bars...\n", num_bars);

    /*
     * PASO 1: DIBUJAR FONDO
     * Llenar toda la pantalla con gris oscuro
     */
    uint16_t bg_color = rgb_to_rgb565(20, 20, 20);  // Gris muy oscuro
    printf("  Drawing background...\n");
    for (y = 0; y < LCD_HEIGHT; y++) {
        for (x = 0; x < LCD_WIDTH; x++) {
            fprintf(fp, "%d\t%d\t%u\n", x, y, bg_color);
        }
    }

    /*
     * PASO 2: DIBUJAR BARRAS DEL HISTOGRAMA
     * Cada barra es un rectángulo de color sólido
     */
    printf("  Drawing histogram bars...\n");
    for (int bar = 0; bar < num_bars; bar++) {
        int x_start = bar * bar_width;                // X inicial de la barra
        int x_end = x_start + bar_width - 2;          // X final (con espaciado)
        int y_start = LCD_HEIGHT - bar_heights[bar];  // Y inicial (desde abajo)
        int y_end = LCD_HEIGHT - 1;                   // Y final (borde inferior)

        // Dibujar rectángulo de la barra
        for (y = y_start; y <= y_end; y++) {
            for (x = x_start; x <= x_end && x < LCD_WIDTH; x++) {
                fprintf(fp, "%d\t%d\t%u\n", x, y, bar_colors[bar]);
            }
        }
    }

    /*
     * PASO 3: DIBUJAR LÍNEAS DE CUADRÍCULA
     * 5 líneas horizontales blancas para referencia
     */
    printf("  Drawing grid lines...\n");
    uint16_t grid_color = rgb_to_rgb565(255, 255, 255);  // Blanco
    for (int i = 0; i <= 4; i++) {
        int grid_y = LCD_HEIGHT - (i * 64);  // Líneas cada 64 píxeles
        if (grid_y >= 0 && grid_y < LCD_HEIGHT) {
            for (x = 0; x < LCD_WIDTH; x++) {
                fprintf(fp, "%d\t%d\t%u\n", x, grid_y, grid_color);
            }
        }
    }

    fclose(fp);
    
    printf("Histogram saved to histogram.cvc\n");
    printf("Total pixels written: %d x %d = %d\n", 
           LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH * LCD_HEIGHT);
    printf("File size: approximately %.1f MB\n", 
           (LCD_WIDTH * LCD_HEIGHT * 20) / 1024.0 / 1024.0);  // Estimación
    
    return 0;
}