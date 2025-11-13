/***************************************************************************//**
*  \file       histogram.c
*  \brief      Implementación de generación de histogramas
*******************************************************************************/

#include "histogram.h"
#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// FUNCIONES AUXILIARES PARA CONVERSIÓN DE COLOR
// ============================================================================

// Helper para escribir un píxel RGB en el buffer de imagen
static void set_pixel(uint8_t *img, int width, int height,
                      int x, int y,
                      uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int idx = (y * width + x) * 3;
    img[idx + 0] = r;
    img[idx + 1] = g;
    img[idx + 2] = b;
}

static uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;
    float r1, g1, b1;

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

    *r = (uint8_t)((r1 + m) * 255);
    *g = (uint8_t)((g1 + m) * 255);
    *b = (uint8_t)((b1 + m) * 255);
}

// ============================================================================
// IMPLEMENTACIÓN: Cálculo de Histograma
// ============================================================================

Histogram* calculate_histogram(const GrayscaleImage *img) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida para calcular histograma\n");
        return NULL;
    }
    
    printf("[MASTER] Calculando histograma de imagen %dx%d\n", 
           img->width, img->height);
    
    Histogram *hist = (Histogram*)calloc(1, sizeof(Histogram));
    if (!hist) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para histograma\n");
        return NULL;
    }
    
    // Inicializar valores
    hist->min_value = 255;
    hist->max_value = 0;
    hist->total_pixels = img->width * img->height;
    
    // Contar frecuencias
    for (int i = 0; i < hist->total_pixels; i++) {
        uint8_t pixel_value = img->data[i];
        hist->bins[pixel_value]++;
        
        if (pixel_value < hist->min_value) hist->min_value = pixel_value;
        if (pixel_value > hist->max_value) hist->max_value = pixel_value;
    }
    
    printf("[MASTER] ✓ Histograma calculado\n");
    
    return hist;
}

void free_histogram(Histogram *hist) {
    if (hist) {
        free(hist);
    }
}

void print_histogram_stats(const Histogram *hist) {
    if (!hist) return;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ESTADÍSTICAS DEL HISTOGRAMA\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Total de píxeles: %d\n", hist->total_pixels);
    printf("  Valor mínimo: %d\n", hist->min_value);
    printf("  Valor máximo: %d\n", hist->max_value);
    
    // Encontrar el valor más frecuente
    uint32_t max_freq = 0;
    int most_common = 0;
    for (int i = 0; i < HISTOGRAM_BINS; i++) {
        if (hist->bins[i] > max_freq) {
            max_freq = hist->bins[i];
            most_common = i;
        }
    }
    printf("  Valor más común: %d (frecuencia: %u)\n", most_common, max_freq);
    printf("═══════════════════════════════════════════════════════════\n\n");
}

// ============================================================================
// IMPLEMENTACIÓN: Generación de Imagen PNG del Histograma
// ============================================================================

bool generate_histogram_png(const Histogram *hist, const char *filename) {
    if (!hist) {
        fprintf(stderr, "[ERROR] Histograma inválido\n");
        return false;
    }
    
    printf("[MASTER] Generando imagen PNG del histograma: %s\n", filename);
    printf("[MASTER]   Eje X: nivel de gris (0 = negro, 255 = blanco)\n");
    printf("[MASTER]   Eje Y: frecuencia de píxeles para cada nivel de gris\n");
    
    const int img_width  = 512;
    const int img_height = 320;
    const int padding    = 40;
    
    // Área de dibujo del histograma (dentro del marco)
    const int plot_left   = padding;
    const int plot_right  = img_width - padding;
    const int plot_top    = padding;
    const int plot_bottom = img_height - padding;
    const int plot_width  = plot_right - plot_left;
    const int plot_height = plot_bottom - plot_top;
    
    // Crear imagen RGB
    uint8_t *img_data = (uint8_t*)calloc(img_width * img_height * 3, sizeof(uint8_t));
    if (!img_data) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para imagen PNG\n");
        return false;
    }
    
    // Fondo blanco
    memset(img_data, 255, img_width * img_height * 3);
    
    // Encontrar frecuencia máxima para normalizar
    uint32_t max_freq = 0;
    for (int i = 0; i < HISTOGRAM_BINS; i++) {
        if (hist->bins[i] > max_freq) {
            max_freq = hist->bins[i];
        }
    }
    if (max_freq == 0) {
        free(img_data);
        fprintf(stderr, "[WARN] Histograma vacío, nada que dibujar\n");
        return false;
    }
    
    // Colores
    const uint8_t bar_r = 50,  bar_g = 100, bar_b = 200; // barras
    const uint8_t frame_r = 0, frame_g = 0, frame_b = 0; // marco/ejes
    const uint8_t grid_r = 200, grid_g = 200, grid_b = 200; // líneas guía
    
    // ===== Líneas de cuadrícula (X e Y) =====
    // X: 0%, 25%, 50%, 75%, 100% de 0–255
    for (int i = 0; i <= 4; i++) {
        int x = plot_left + (plot_width * i) / 4;
        for (int y = plot_top; y <= plot_bottom; y++) {
            set_pixel(img_data, img_width, img_height, x, y, grid_r, grid_g, grid_b);
        }
        int gray_value = (255 * i) / 4;
        printf("[MASTER]   Marca X %d%% ≈ nivel gris %d\n", i * 25, gray_value);
    }
    
    // Y: 0%, 25%, 50%, 75%, 100% de frecuencia máxima
    for (int i = 0; i <= 4; i++) {
        int y = plot_bottom - (plot_height * i) / 4;
        for (int x = plot_left; x <= plot_right; x++) {
            set_pixel(img_data, img_width, img_height, x, y, grid_r, grid_g, grid_b);
        }
        uint32_t freq_value = (max_freq * i) / 4;
        printf("[MASTER]   Marca Y %d%% ≈ frecuencia %u\n", i * 25, freq_value);
    }
    
    // ===== Dibujar barras del histograma =====
    int bar_width = plot_width / HISTOGRAM_BINS;
    if (bar_width < 1) bar_width = 1;
    
    for (int i = 0; i < HISTOGRAM_BINS; i++) {
        int bar_height = (int)((float)hist->bins[i] / max_freq * (float)plot_height);
        if (bar_height <= 0) continue;
        
        int x_base = plot_left + i * bar_width;
        int y_start = plot_bottom - bar_height;
        int y_end   = plot_bottom;
        
        for (int y = y_start; y < y_end; y++) {
            if (y < plot_top || y > plot_bottom) continue;
            for (int bx = 0; bx < bar_width; bx++) {
                int px = x_base + bx;
                if (px < plot_left || px > plot_right) continue;
                set_pixel(img_data, img_width, img_height, px, y, bar_r, bar_g, bar_b);
            }
        }
    }
    
    // ===== Marco que encierra el histograma (ejes X/Y incluidos) =====
    // Línea superior e inferior
    for (int x = plot_left; x <= plot_right; x++) {
        set_pixel(img_data, img_width, img_height, x, plot_top,    frame_r, frame_g, frame_b);
        set_pixel(img_data, img_width, img_height, x, plot_bottom, frame_r, frame_g, frame_b);
    }
    // Línea izquierda y derecha
    for (int y = plot_top; y <= plot_bottom; y++) {
        set_pixel(img_data, img_width, img_height, plot_left,  y, frame_r, frame_g, frame_b);
        set_pixel(img_data, img_width, img_height, plot_right, y, frame_r, frame_g, frame_b);
    }
    
    // Guardar imagen
    int result = stbi_write_png(filename, img_width, img_height, 3, img_data, img_width * 3);
    free(img_data);
    
    if (!result) {
        fprintf(stderr, "[ERROR] No se pudo guardar imagen PNG del histograma\n");
        return false;
    }
    
    printf("[MASTER] ✓ Imagen PNG del histograma generada\n");
    return true;
}

// ============================================================================
// IMPLEMENTACIÓN: Generación de Archivo .cvc
// ============================================================================

bool generate_histogram_cvc(const Histogram *hist, const char *filename) {
    if (!hist) {
        fprintf(stderr, "[ERROR] Histograma inválido\n");
        return false;
    }
    
    printf("[MASTER] Generando archivo .cvc del histograma: %s\n", filename);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[ERROR] No se pudo crear archivo .cvc\n");
        return false;
    }
    
    // Escribir header
    fprintf(fp, "pixelx\tpixely\tvalue\n");
    
    // Colores
    uint16_t bg_color = rgb_to_rgb565(20, 20, 20);  // Fondo gris oscuro
    uint16_t grid_color = rgb_to_rgb565(255, 255, 255);  // Grid blanco
    
    // Encontrar frecuencia máxima para normalizar
    uint32_t max_freq = 0;
    for (int i = 0; i < HISTOGRAM_BINS; i++) {
        if (hist->bins[i] > max_freq) {
            max_freq = hist->bins[i];
        }
    }
    
    printf("[MASTER]   Frecuencia máxima: %u\n", max_freq);
    printf("[MASTER]   Escribiendo fondo...\n");
    
    // PASO 1: Dibujar fondo
    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            fprintf(fp, "%d\t%d\t%u\n", x, y, bg_color);
        }
    }
    
    printf("[MASTER]   Escribiendo barras del histograma...\n");
    
    // PASO 2: Dibujar barras del histograma
    int num_bars = HISTOGRAM_BINS;  // 256 barras (una por cada nivel de gris)
    
    // Agrupar bins para que quepan en el LCD (240 pixeles de ancho)
    int bins_per_bar = (num_bars + LCD_WIDTH - 1) / LCD_WIDTH;  // Redondear arriba
    int actual_bars = (num_bars + bins_per_bar - 1) / bins_per_bar;
    
    for (int bar = 0; bar < actual_bars && bar < LCD_WIDTH; bar++) {
        // Calcular altura promedio de los bins agrupados
        uint32_t total_freq = 0;
        int bin_start = bar * bins_per_bar;
        int bin_end = (bar + 1) * bins_per_bar;
        if (bin_end > num_bars) bin_end = num_bars;
        
        for (int b = bin_start; b < bin_end; b++) {
            total_freq += hist->bins[b];
        }
        
        uint32_t avg_freq = total_freq / bins_per_bar;
        
        // Normalizar altura a LCD_HEIGHT
        int bar_height = (int)((float)avg_freq / max_freq * (LCD_HEIGHT - 10));
        if (bar_height > LCD_HEIGHT - 10) bar_height = LCD_HEIGHT - 10;
        
        // Calcular color (arcoíris)
        uint8_t r, g, b;
        float hue = (360.0 * bar) / actual_bars;
        hsv_to_rgb(hue, 0.9, 0.9, &r, &g, &b);
        uint16_t bar_color = rgb_to_rgb565(r, g, b);
        
        // Dibujar barra
        int y_start = LCD_HEIGHT - bar_height;
        int y_end = LCD_HEIGHT - 1;
        
        for (int y = y_start; y <= y_end; y++) {
            fprintf(fp, "%d\t%d\t%u\n", bar, y, bar_color);
        }
    }
    
    printf("[MASTER]   Escribiendo líneas de cuadrícula...\n");
    
    // PASO 3: Dibujar líneas de cuadrícula
    for (int i = 0; i <= 4; i++) {
        int grid_y = LCD_HEIGHT - (i * 64);
        if (grid_y >= 0 && grid_y < LCD_HEIGHT) {
            for (int x = 0; x < LCD_WIDTH; x++) {
                fprintf(fp, "%d\t%d\t%u\n", x, grid_y, grid_color);
            }
        }
    }
    
    fclose(fp);
    
    printf("[MASTER] ✓ Archivo .cvc generado exitosamente\n");
    printf("[MASTER]   Total de píxeles escritos: %d x %d = %d\n",
           LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH * LCD_HEIGHT);
    
    return true;
}