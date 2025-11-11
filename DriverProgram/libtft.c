/***************************************************************************//**
*  \file       libtft.c
*  \details    Implementation of TFT Display Library
*******************************************************************************/
#include "libtft.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>

#define TFT_DEVICE_PATH "/dev/tft_device"
#define TFT_IOCTL_RESET     _IO('T', 0)
#define TFT_IOCTL_DRAW_IMAGE _IO('T', 1)
#define MAX_PIXELS_BUFFER 1024

struct pixel_data {
    uint16_t x;
    uint16_t y;
    uint16_t color;
} __attribute__((packed));

static char last_error[256] = {0};

static void set_error(const char *msg)
{
    snprintf(last_error, sizeof(last_error), "%s", msg);
}

const char* tft_get_error(void)
{
    return last_error;
}

tft_handle_t* tft_init(void)
{
    tft_handle_t *handle;
    
    handle = (tft_handle_t*)malloc(sizeof(tft_handle_t));
    if (!handle) {
        set_error("Failed to allocate memory for handle");
        return NULL;
    }
    
    handle->fd = open(TFT_DEVICE_PATH, O_RDWR);
    if (handle->fd < 0) {
        set_error("Failed to open TFT device");
        free(handle);
        return NULL;
    }
    
    handle->is_open = 1;
    set_error("Success");
    return handle;
}

int tft_close(tft_handle_t *handle)
{
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    close(handle->fd);
    handle->is_open = 0;
    free(handle);
    
    set_error("Success");
    return TFT_SUCCESS;
}

int tft_reset(tft_handle_t *handle)
{
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    if (ioctl(handle->fd, TFT_IOCTL_RESET) < 0) {
        set_error("Failed to reset display");
        return TFT_ERROR_IO;
    }
    
    set_error("Success");
    return TFT_SUCCESS;
}

int tft_draw_pixel(tft_handle_t *handle, uint16_t x, uint16_t y, uint16_t color)
{
    struct pixel_data pixel;
    
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        set_error("Coordinates out of bounds");
        return TFT_ERROR_INVALID;
    }
    
    pixel.x = x;
    pixel.y = y;
    pixel.color = color;
    
    if (write(handle->fd, &pixel, sizeof(pixel)) != sizeof(pixel)) {
        set_error("Failed to write pixel");
        return TFT_ERROR_IO;
    }
    
    return TFT_SUCCESS;
}

int tft_fill_screen(tft_handle_t *handle, uint16_t color)
{
    struct pixel_data *pixels;
    int total_pixels = TFT_WIDTH * TFT_HEIGHT;
    int pixels_written = 0;
    int chunk_size = MAX_PIXELS_BUFFER;
    int i;
    
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    pixels = (struct pixel_data*)malloc(chunk_size * sizeof(struct pixel_data));
    if (!pixels) {
        set_error("Failed to allocate memory");
        return TFT_ERROR_MEMORY;
    }
    
    while (pixels_written < total_pixels) {
        int to_write = (total_pixels - pixels_written > chunk_size) ? 
                       chunk_size : (total_pixels - pixels_written);
        
        for (i = 0; i < to_write; i++) {
            int pixel_num = pixels_written + i;
            pixels[i].x = pixel_num % TFT_WIDTH;
            pixels[i].y = pixel_num / TFT_WIDTH;
            pixels[i].color = color;
        }
        
        if (write(handle->fd, pixels, to_write * sizeof(struct pixel_data)) < 0) {
            set_error("Failed to write pixels");
            free(pixels);
            return TFT_ERROR_IO;
        }
        
        pixels_written += to_write;
    }
    
    free(pixels);
    set_error("Success");
    return TFT_SUCCESS;
}

int tft_clear(tft_handle_t *handle)
{
    return tft_fill_screen(handle, TFT_COLOR_BLACK);
}

int tft_load_cvc_file(tft_handle_t *handle, const char *filename)
{
    FILE *fp;
    char line[256];
    struct pixel_data *pixels;
    int pixel_count = 0;
    int capacity = 10000;
    int x, y, color;
    int chunks_written = 0;
    
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    fp = fopen(filename, "r");
    if (!fp) {
        set_error("Failed to open CVC file");
        return TFT_ERROR_IO;
    }
    
    pixels = (struct pixel_data*)malloc(capacity * sizeof(struct pixel_data));
    if (!pixels) {
        set_error("Failed to allocate memory");
        fclose(fp);
        return TFT_ERROR_MEMORY;
    }
    
    // Skip header
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d\t%d\t%d", &x, &y, &color) == 3) {
            if (pixel_count >= capacity) {
                capacity *= 2;
                struct pixel_data *new_pixels = (struct pixel_data*)realloc(
                    pixels, capacity * sizeof(struct pixel_data));
                if (!new_pixels) {
                    set_error("Failed to reallocate memory");
                    free(pixels);
                    fclose(fp);
                    return TFT_ERROR_MEMORY;
                }
                pixels = new_pixels;
            }
            pixels[pixel_count].x = (uint16_t)x;
            pixels[pixel_count].y = (uint16_t)y;
            pixels[pixel_count].color = (uint16_t)color;
            pixel_count++;
        }
    }
    
    fclose(fp);
    
    // Write in chunks
    int offset = 0;
    while (offset < pixel_count) {
        int to_write = (pixel_count - offset > MAX_PIXELS_BUFFER) ? 
                       MAX_PIXELS_BUFFER : (pixel_count - offset);
        
        if (write(handle->fd, &pixels[offset], to_write * sizeof(struct pixel_data)) < 0) {
            set_error("Failed to write pixels");
            free(pixels);
            return TFT_ERROR_IO;
        }
        
        offset += to_write;
        chunks_written++;
    }
    
    free(pixels);
    set_error("Success");
    return TFT_SUCCESS;
}

int tft_fill_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color)
{
    struct pixel_data *pixels;
    int total_pixels = width * height;
    int i, px, py;
    
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    if (x + width > TFT_WIDTH || y + height > TFT_HEIGHT) {
        set_error("Rectangle out of bounds");
        return TFT_ERROR_INVALID;
    }
    
    pixels = (struct pixel_data*)malloc(total_pixels * sizeof(struct pixel_data));
    if (!pixels) {
        set_error("Failed to allocate memory");
        return TFT_ERROR_MEMORY;
    }
    
    i = 0;
    for (py = 0; py < height; py++) {
        for (px = 0; px < width; px++) {
            pixels[i].x = x + px;
            pixels[i].y = y + py;
            pixels[i].color = color;
            i++;
        }
    }
    
    // Write in chunks
    int offset = 0;
    while (offset < total_pixels) {
        int to_write = (total_pixels - offset > MAX_PIXELS_BUFFER) ? 
                       MAX_PIXELS_BUFFER : (total_pixels - offset);
        
        if (write(handle->fd, &pixels[offset], to_write * sizeof(struct pixel_data)) < 0) {
            set_error("Failed to write pixels");
            free(pixels);
            return TFT_ERROR_IO;
        }
        
        offset += to_write;
    }
    
    free(pixels);
    set_error("Success");
    return TFT_SUCCESS;
}

int tft_draw_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color)
{
    int i;
    
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    // Top and bottom lines
    for (i = 0; i < width; i++) {
        tft_draw_pixel(handle, x + i, y, color);
        tft_draw_pixel(handle, x + i, y + height - 1, color);
    }
    
    // Left and right lines
    for (i = 0; i < height; i++) {
        tft_draw_pixel(handle, x, y + i, color);
        tft_draw_pixel(handle, x + width - 1, y + i, color);
    }
    
    set_error("Success");
    return TFT_SUCCESS;
}

static void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
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

int tft_draw_histogram(tft_handle_t *handle, const int *values, int num_bars, int max_value)
{
    int i;
    int bar_width;
    int bar_spacing = 2;
    uint16_t bg_color = tft_rgb_to_color(20, 20, 20);
    
    if (!handle || !handle->is_open) {
        set_error("Invalid handle");
        return TFT_ERROR_INVALID;
    }
    
    if (num_bars <= 0 || num_bars > TFT_WIDTH || max_value <= 0) {
        set_error("Invalid histogram parameters");
        return TFT_ERROR_INVALID;
    }
    
    // Clear screen
    tft_fill_screen(handle, bg_color);
    
    bar_width = (TFT_WIDTH / num_bars) - bar_spacing;
    if (bar_width < 1) bar_width = 1;
    
    // Draw bars
    for (i = 0; i < num_bars; i++) {
        int bar_height = (values[i] * (TFT_HEIGHT - 40)) / max_value;
        if (bar_height > TFT_HEIGHT - 40) bar_height = TFT_HEIGHT - 40;
        
        int x = i * (bar_width + bar_spacing);
        int y = TFT_HEIGHT - bar_height - 20;
        
        // Rainbow colors
        uint8_t r, g, b;
        float hue = (360.0 * i) / num_bars;
        hsv_to_rgb(hue, 0.9, 0.9, &r, &g, &b);
        uint16_t color = tft_rgb_to_color(r, g, b);
        
        tft_fill_rect(handle, x, y, bar_width, bar_height, color);
    }
    
    // Draw grid lines
    for (i = 0; i <= 4; i++) {
        int grid_y = TFT_HEIGHT - 20 - (i * ((TFT_HEIGHT - 40) / 4));
        tft_fill_rect(handle, 0, grid_y, TFT_WIDTH, 1, TFT_COLOR_WHITE);
    }
    
    set_error("Success");
    return TFT_SUCCESS;
}

uint16_t tft_rgb_to_color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}