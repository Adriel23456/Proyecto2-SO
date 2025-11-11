/***************************************************************************//**
*  \file       test_tft.c
*  \details    Test program using libtft library
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libtft.h"

void print_usage(const char *prog)
{
    printf("Usage: %s <command> [args]\n", prog);
    printf("Commands:\n");
    printf("  reset              - Reset display\n");
    printf("  clear              - Clear display (black)\n");
    printf("  fill <color>       - Fill with color (hex RGB565)\n");
    printf("  cvc <file>         - Load CVC file\n");
    printf("  histogram <file>   - Draw histogram from data file\n");
    printf("  rect <x> <y> <w> <h> <color> - Draw rectangle\n");
    printf("  demo               - Run demo sequence\n");
}

int main(int argc, char *argv[])
{
    tft_handle_t *tft;
    int ret;
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    tft = tft_init();
    if (!tft) {
        fprintf(stderr, "Error: %s\n", tft_get_error());
        return 1;
    }
    
    printf("TFT initialized successfully\n");
    
    if (strcmp(argv[1], "reset") == 0) {
        printf("Resetting display...\n");
        ret = tft_reset(tft);
        
    } else if (strcmp(argv[1], "clear") == 0) {
        printf("Clearing display...\n");
        ret = tft_clear(tft);
        
    } else if (strcmp(argv[1], "fill") == 0 && argc >= 3) {
        uint16_t color = (uint16_t)strtol(argv[2], NULL, 16);
        printf("Filling with color 0x%04X...\n", color);
        ret = tft_fill_screen(tft, color);
        
    } else if (strcmp(argv[1], "cvc") == 0 && argc >= 3) {
        printf("Loading CVC file: %s...\n", argv[2]);
        ret = tft_load_cvc_file(tft, argv[2]);
        
    } else if (strcmp(argv[1], "histogram") == 0 && argc >= 3) {
        FILE *fp = fopen(argv[2], "r");
        if (!fp) {
            fprintf(stderr, "Failed to open data file\n");
            tft_close(tft);
            return 1;
        }
        
        int values[256];
        int num_values = 0;
        while (fscanf(fp, "%d", &values[num_values]) == 1 && num_values < 256) {
            num_values++;
        }
        fclose(fp);
        
        if (num_values == 0) {
            fprintf(stderr, "No data in file\n");
            tft_close(tft);
            return 1;
        }
        
        // Find max value
        int max_val = values[0];
        for (int i = 1; i < num_values; i++) {
            if (values[i] > max_val) max_val = values[i];
        }
        
        printf("Drawing histogram with %d bars (max: %d)...\n", num_values, max_val);
        ret = tft_draw_histogram(tft, values, num_values, max_val);
        
    } else if (strcmp(argv[1], "rect") == 0 && argc >= 7) {
        uint16_t x = atoi(argv[2]);
        uint16_t y = atoi(argv[3]);
        uint16_t w = atoi(argv[4]);
        uint16_t h = atoi(argv[5]);
        uint16_t color = (uint16_t)strtol(argv[6], NULL, 16);
        printf("Drawing rectangle at (%d,%d) size %dx%d color 0x%04X...\n", x, y, w, h, color);
        ret = tft_fill_rect(tft, x, y, w, h, color);
        
    } else if (strcmp(argv[1], "demo") == 0) {
        printf("Running demo sequence...\n");
        
        printf("1. Clear screen...\n");
        tft_clear(tft);
        sleep(1);
        
        printf("2. Red rectangle...\n");
        tft_fill_rect(tft, 20, 20, 100, 100, TFT_COLOR_RED);
        sleep(1);
        
        printf("3. Green rectangle...\n");
        tft_fill_rect(tft, 120, 120, 100, 100, TFT_COLOR_GREEN);
        sleep(1);
        
        printf("4. Blue rectangle...\n");
        tft_fill_rect(tft, 70, 200, 100, 100, TFT_COLOR_BLUE);
        sleep(2);
        
        printf("5. Sample histogram...\n");
        int sample_data[] = {50, 80, 120, 90, 150, 70, 110, 95, 130, 85, 
                            60, 100, 140, 75, 115, 88, 125, 92, 105, 78};
        tft_draw_histogram(tft, sample_data, 20, 150);
        
        ret = TFT_SUCCESS;
        
    } else {
        print_usage(argv[0]);
        tft_close(tft);
        return 1;
    }
    
    if (ret != TFT_SUCCESS) {
        fprintf(stderr, "Error: %s\n", tft_get_error());
        tft_close(tft);
        return 1;
    }
    
    printf("Operation completed successfully!\n");
    tft_close(tft);
    return 0;
}