/***************************************************************************//**
*  \file       libtft.h
*  \details    Public API for TFT Display Library
*  \brief      User-space library for TFT driver interaction
*******************************************************************************/
#ifndef LIBTFT_H
#define LIBTFT_H

#include <stdint.h>

// Display dimensions
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Color definitions (RGB565)
#define TFT_COLOR_BLACK   0x0000
#define TFT_COLOR_WHITE   0xFFFF
#define TFT_COLOR_RED     0xF800
#define TFT_COLOR_GREEN   0x07E0
#define TFT_COLOR_BLUE    0x001F
#define TFT_COLOR_YELLOW  0xFFE0
#define TFT_COLOR_CYAN    0x07FF
#define TFT_COLOR_MAGENTA 0xF81F

// Error codes
#define TFT_SUCCESS       0
#define TFT_ERROR_DEVICE  -1
#define TFT_ERROR_MEMORY  -2
#define TFT_ERROR_INVALID -3
#define TFT_ERROR_IO      -4

// Library handle
typedef struct {
    int fd;
    int is_open;
} tft_handle_t;

/***************************************************************************//**
* \brief Initialize TFT display connection
* \return Pointer to handle on success, NULL on failure
*******************************************************************************/
tft_handle_t* tft_init(void);

/***************************************************************************//**
* \brief Close TFT display connection
* \param handle TFT handle
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_close(tft_handle_t *handle);

/***************************************************************************//**
* \brief Reset display to initial state
* \param handle TFT handle
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_reset(tft_handle_t *handle);

/***************************************************************************//**
* \brief Draw single pixel
* \param handle TFT handle
* \param x X coordinate
* \param y Y coordinate
* \param color RGB565 color value
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_draw_pixel(tft_handle_t *handle, uint16_t x, uint16_t y, uint16_t color);

/***************************************************************************//**
* \brief Fill entire screen with color
* \param handle TFT handle
* \param color RGB565 color value
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_fill_screen(tft_handle_t *handle, uint16_t color);

/***************************************************************************//**
* \brief Clear screen (fill with black)
* \param handle TFT handle
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_clear(tft_handle_t *handle);

/***************************************************************************//**
* \brief Load and display image from CVC file
* \param handle TFT handle
* \param filename Path to .cvc file
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_load_cvc_file(tft_handle_t *handle, const char *filename);

/***************************************************************************//**
* \brief Draw histogram on display
* \param handle TFT handle
* \param values Array of histogram values
* \param num_bars Number of bars in histogram
* \param max_value Maximum value for scaling
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_draw_histogram(tft_handle_t *handle, const int *values, int num_bars, int max_value);

/***************************************************************************//**
* \brief Draw rectangle
* \param handle TFT handle
* \param x X coordinate
* \param y Y coordinate
* \param width Width of rectangle
* \param height Height of rectangle
* \param color RGB565 color value
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_draw_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color);

/***************************************************************************//**
* \brief Draw filled rectangle
* \param handle TFT handle
* \param x X coordinate
* \param y Y coordinate
* \param width Width of rectangle
* \param height Height of rectangle
* \param color RGB565 color value
* \return TFT_SUCCESS on success, error code on failure
*******************************************************************************/
int tft_fill_rect(tft_handle_t *handle, uint16_t x, uint16_t y, 
                  uint16_t width, uint16_t height, uint16_t color);

/***************************************************************************//**
* \brief Convert RGB888 to RGB565
* \param r Red component (0-255)
* \param g Green component (0-255)
* \param b Blue component (0-255)
* \return RGB565 color value
*******************************************************************************/
uint16_t tft_rgb_to_color(uint8_t r, uint8_t g, uint8_t b);

/***************************************************************************//**
* \brief Get last error message
* \return Error message string
*******************************************************************************/
const char* tft_get_error(void);

#endif /* LIBTFT_H */