#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>

#define TFT_IOCTL_RESET     _IO('T', 0)
#define TFT_IOCTL_DRAW_IMAGE _IO('T', 1)

struct pixel_data {
    uint16_t x;
    uint16_t y;
    uint16_t color;
} __attribute__((packed));

int main(int argc, char *argv[])
{
    int fd;
    FILE *fp;
    char line[256];
    struct pixel_data *pixels;
    int pixel_count = 0;
    int capacity = 10000;
    int x, y, color;

    if (argc < 2) {
        printf("Usage: %s <file.cvc> | reset\n", argv[0]);
        return 1;
    }

    fd = open("/dev/tft_device", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    if (strcmp(argv[1], "reset") == 0) {
        printf("Resetting display...\n");
        ioctl(fd, TFT_IOCTL_RESET);
        close(fd);
        return 0;
    }

    fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Failed to open CVC file");
        close(fd);
        return 1;
    }

    pixels = malloc(capacity * sizeof(struct pixel_data));
    if (!pixels) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        close(fd);
        return 1;
    }

    // Skip header line
    fgets(line, sizeof(line), fp);

    printf("Loading image from %s...\n", argv[1]);

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d\t%d\t%d", &x, &y, &color) == 3) {
            if (pixel_count >= capacity) {
                capacity *= 2;
                pixels = realloc(pixels, capacity * sizeof(struct pixel_data));
                if (!pixels) {
                    fprintf(stderr, "Memory reallocation failed\n");
                    fclose(fp);
                    close(fd);
                    return 1;
                }
            }
            pixels[pixel_count].x = (uint16_t)x;
            pixels[pixel_count].y = (uint16_t)y;
            pixels[pixel_count].color = (uint16_t)color;
            pixel_count++;
        }
    }

    fclose(fp);

    printf("Loaded %d pixels. Drawing to display...\n", pixel_count);

    ioctl(fd, TFT_IOCTL_DRAW_IMAGE);

    // Write in chunks
    int chunk_size = 1024;
    int offset = 0;
    while (offset < pixel_count) {
        int to_write = (pixel_count - offset > chunk_size) ? chunk_size : (pixel_count - offset);
        int written = write(fd, &pixels[offset], to_write * sizeof(struct pixel_data));
        if (written < 0) {
            perror("Write failed");
            break;
        }
        offset += to_write;
        printf("Progress: %d/%d pixels\r", offset, pixel_count);
        fflush(stdout);
    }

    printf("\nImage drawn successfully!\n");

    free(pixels);
    close(fd);
    return 0;
}