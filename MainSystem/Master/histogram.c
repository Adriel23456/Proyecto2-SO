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
// Helpers de dibujo básico (para el PNG)
// ============================================================================

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

// Fuente 5x7 muy simple para unas pocas letras y dígitos
typedef struct {
    char ch;
    uint8_t rows[7];   // 5 bits útiles por fila (de izquierda a derecha)
} Glyph5x7;

static const Glyph5x7 FONT_5x7[] = {
    // A
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    // C
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
    // D
    { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
    // E
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
    // F
    { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    // G
    { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E } },
    // I
    { 'I', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F } },
    // L
    { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
    // N
    { 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
    // R
    { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    // S
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    // U
    { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    // V
    { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },

    // Dígitos 0,2,5
    { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
    { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
    { '5', { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E } },

    // Espacio
    { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },

    // Guion '-'
    { '-', { 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00 } },

    // Paréntesis
    { '(', { 0x06, 0x08, 0x10, 0x10, 0x10, 0x08, 0x06 } },
    { ')', { 0x0C, 0x02, 0x01, 0x01, 0x01, 0x02, 0x0C } },
};

static const int FONT_COUNT_5x7 = sizeof(FONT_5x7) / sizeof(FONT_5x7[0]);

static const Glyph5x7* find_glyph_5x7(char c)
{
    for (int i = 0; i < FONT_COUNT_5x7; i++) {
        if (FONT_5x7[i].ch == c) return &FONT_5x7[i];
    }
    return NULL; // sin glyph -> no dibujar
}

static void draw_char_5x7(uint8_t *img, int width, int height,
                          int x, int y, char c,
                          uint8_t r, uint8_t g, uint8_t b)
{
    const Glyph5x7 *glyph = find_glyph_5x7(c);
    if (!glyph) return;

    for (int row = 0; row < 7; row++) {
        uint8_t bits = glyph->rows[row];
        for (int col = 0; col < 5; col++) {
            // Bit 4 = columna izquierda, bit 0 = derecha
            if (bits & (1 << (4 - col))) {
                set_pixel(img, width, height, x + col, y + row, r, g, b);
            }
        }
    }
}

static void draw_text_5x7(uint8_t *img, int width, int height,
                          int x, int y, const char *text,
                          uint8_t r, uint8_t g, uint8_t b)
{
    const int char_spacing = 6; // 5 px letra + 1 px espacio
    for (const char *p = text; *p; ++p) {
        draw_char_5x7(img, width, height, x, y, *p, r, g, b);
        x += char_spacing;
    }
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
    
    const int img_width  = 512;
    const int img_height = 320;
    const int padding    = 40;
    
    // Área del marco
    const int plot_left   = padding;
    const int plot_right  = img_width - padding;
    const int plot_top    = padding;
    const int plot_bottom = img_height - padding;
    const int plot_width  = plot_right - plot_left;
    const int plot_height = plot_bottom - plot_top;
    
    // Área para las barras (evitando el marco)
    const int bar_left   = plot_left + 1;
    const int bar_right  = plot_right - 1;
    const int bar_top    = plot_top + 1;
    const int bar_bottom = plot_bottom;  // SIN -1 para llenar hasta abajo
    const int bar_width  = bar_right - bar_left;
    const int bar_height = bar_bottom - bar_top;
    
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
    const uint8_t bar_r   = 50,  bar_g   = 100, bar_b   = 200;
    const uint8_t frame_r = 0,   frame_g = 0,   frame_b = 0;
    const uint8_t grid_r  = 200, grid_g  = 200, grid_b  = 200;
    const uint8_t text_r  = 0,   text_g  = 0,   text_b  = 0;
    
    // ===== Líneas de cuadrícula =====
    for (int i = 0; i <= 4; i++) {
        int x = plot_left + (plot_width * i) / 4;
        for (int y = plot_top; y <= plot_bottom; y++) {
            set_pixel(img_data, img_width, img_height, x, y, grid_r, grid_g, grid_b);
        }
    }
    
    for (int i = 0; i <= 4; i++) {
        int y = plot_bottom - (plot_height * i) / 4;
        for (int x = plot_left; x <= plot_right; x++) {
            set_pixel(img_data, img_width, img_height, x, y, grid_r, grid_g, grid_b);
        }
    }
    
    // ===== Dibujar barras del histograma =====
    for (int i = 0; i < HISTOGRAM_BINS; i++) {
        int barra_altura = (int)((float)hist->bins[i] / max_freq * (float)bar_height);
        if (barra_altura <= 0) continue;
        
        int y_start = bar_bottom - barra_altura;
        int y_end   = bar_bottom;
        
        // Mapear usando bar_width para que llegue hasta bar_right
        int x0 = bar_left + (i * bar_width) / HISTOGRAM_BINS;
        int x1 = bar_left + ((i + 1) * bar_width) / HISTOGRAM_BINS;
        if (x1 > bar_right) x1 = bar_right;
        if (x1 <= x0) x1 = x0 + 1;
        
        for (int x = x0; x < x1; x++) {
            for (int y = y_start; y < y_end; y++) {
                set_pixel(img_data, img_width, img_height, x, y, bar_r, bar_g, bar_b);
            }
        }
    }
    
    // ===== Marco (se dibuja DESPUÉS de las barras) =====
    for (int x = plot_left; x <= plot_right; x++) {
        set_pixel(img_data, img_width, img_height, x, plot_top,    frame_r, frame_g, frame_b);
        set_pixel(img_data, img_width, img_height, x, plot_bottom, frame_r, frame_g, frame_b);
    }
    for (int y = plot_top; y <= plot_bottom; y++) {
        set_pixel(img_data, img_width, img_height, plot_left,  y, frame_r, frame_g, frame_b);
        set_pixel(img_data, img_width, img_height, plot_right, y, frame_r, frame_g, frame_b);
    }
    
    // ===== Etiquetas de los ejes =====
    const char *xlabel = "NIVEL DE GRIS (0-255)";
    const char *ylabel = "FRECUENCIA";
    
    int xlabel_len = (int)strlen(xlabel);
    int xlabel_x = img_width / 2 - (xlabel_len * 6) / 2;
    int xlabel_y = plot_bottom + (padding / 2) - 4;
    
    if (xlabel_y + 7 < img_height) {
        draw_text_5x7(img_data, img_width, img_height,
                      xlabel_x, xlabel_y, xlabel,
                      text_r, text_g, text_b);
    }
    
    int ylabel_x = 5;
    int ylabel_y = plot_top + (plot_height / 2) - 4;
    
    if (ylabel_y + 7 < img_height) {
        draw_text_5x7(img_data, img_width, img_height,
                      ylabel_x, ylabel_y, ylabel,
                      text_r, text_g, text_b);
    }
    
    // Guardar imagen
    int result = stbi_write_png(filename, img_width, img_height, 3, img_data, img_width * 3);
    free(img_data);
    
    if (!result) {
        fprintf(stderr, "[ERROR] No se pudo guardar imagen PNG del histograma\n");
        return false;
    }
    
    printf("[MASTER] ✓ Imagen PNG del histograma generada (con ejes etiquetados)\n");
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
    
    // PASO 2: Dibujar barras del histograma mapeando 0–255 a 0–(LCD_WIDTH-1)
    if (max_freq == 0) {
        printf("[MASTER]   Histograma vacío, no se dibujan barras\n");
    } else {
        for (int x = 0; x < LCD_WIDTH; x++) {
            // Rango de bins (niveles de gris) que caen en esta columna x
            int bin_start = (x * HISTOGRAM_BINS) / LCD_WIDTH;
            int bin_end   = ((x + 1) * HISTOGRAM_BINS) / LCD_WIDTH;
            if (bin_end <= bin_start) bin_end = bin_start + 1;
            if (bin_end > HISTOGRAM_BINS) bin_end = HISTOGRAM_BINS;

            uint32_t total_freq = 0;
            int bins_in_group = 0;
            for (int b = bin_start; b < bin_end; b++) {
                total_freq += hist->bins[b];
                bins_in_group++;
            }

            uint32_t avg_freq = (bins_in_group > 0) ? (total_freq / bins_in_group) : 0;

            // Normalizar altura a LCD_HEIGHT
            int bar_height = (int)((float)avg_freq / max_freq * (LCD_HEIGHT - 10));
            if (bar_height < 0) bar_height = 0;
            if (bar_height > LCD_HEIGHT - 10) bar_height = LCD_HEIGHT - 10;

            // Color arcoíris según posición x (no según bar)
            uint8_t r, g, b;
            float hue = (360.0f * x) / (float)LCD_WIDTH;
            hsv_to_rgb(hue, 0.9f, 0.9f, &r, &g, &b);
            uint16_t bar_color = rgb_to_rgb565(r, g, b);

            int y_start = LCD_HEIGHT - bar_height;
            int y_end   = LCD_HEIGHT - 1;

            for (int y = y_start; y <= y_end; y++) {
                if (y < 0 || y >= LCD_HEIGHT) continue;
                fprintf(fp, "%d\t%d\t%u\n", x, y, bar_color);
            }
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