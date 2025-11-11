#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// Convert RGB to RGB565
uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// HSV to RGB conversion
void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
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

int main(void)
{
    FILE *fp;
    int x, y;
    int num_bars = 20;
    int bar_width = LCD_WIDTH / num_bars;
    int bar_heights[20];
    uint16_t bar_colors[20];

    fp = fopen("histogram.cvc", "w");
    if (!fp) {
        perror("Failed to create histogram.cvc");
        return 1;
    }

    fprintf(fp, "pixelx\tpixely\tvalue\n");

    // Generate random bar heights and rainbow colors
    srand(42);
    for (int i = 0; i < num_bars; i++) {
        bar_heights[i] = 50 + rand() % 200;
        
        uint8_t r, g, b;
        float hue = (360.0 * i) / num_bars;
        hsv_to_rgb(hue, 0.9, 0.9, &r, &g, &b);
        bar_colors[i] = rgb_to_rgb565(r, g, b);
    }

    printf("Generating colorful histogram with %d bars...\n", num_bars);

    // Draw background (dark gray)
    uint16_t bg_color = rgb_to_rgb565(20, 20, 20);
    for (y = 0; y < LCD_HEIGHT; y++) {
        for (x = 0; x < LCD_WIDTH; x++) {
            fprintf(fp, "%d\t%d\t%u\n", x, y, bg_color);
        }
    }

    // Draw histogram bars
    for (int bar = 0; bar < num_bars; bar++) {
        int x_start = bar * bar_width;
        int x_end = x_start + bar_width - 2;
        int y_start = LCD_HEIGHT - bar_heights[bar];
        int y_end = LCD_HEIGHT - 1;

        for (y = y_start; y <= y_end; y++) {
            for (x = x_start; x <= x_end && x < LCD_WIDTH; x++) {
                fprintf(fp, "%d\t%d\t%u\n", x, y, bar_colors[bar]);
            }
        }
    }

    // Draw grid lines (white)
    uint16_t grid_color = rgb_to_rgb565(255, 255, 255);
    for (int i = 0; i <= 4; i++) {
        int grid_y = LCD_HEIGHT - (i * 64);
        if (grid_y >= 0 && grid_y < LCD_HEIGHT) {
            for (x = 0; x < LCD_WIDTH; x++) {
                fprintf(fp, "%d\t%d\t%u\n", x, grid_y, grid_color);
            }
        }
    }

    fclose(fp);
    printf("Histogram saved to histogram.cvc\n");
    printf("Total pixels: %d x %d = %d\n", LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH * LCD_HEIGHT);
    
    return 0;
}