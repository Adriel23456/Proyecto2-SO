#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define TFT_IOCTL_RESET     _IO('T', 0)
#define TFT_IOCTL_DRAW_CIRCLE _IO('T', 1)

int main(int argc, char *argv[])
{
    int fd;

    fd = open("/dev/tft_device", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    if (argc > 1 && argv[1][0] == 'r') {
        printf("Resetting display...\n");
        ioctl(fd, TFT_IOCTL_RESET);
    } else {
        printf("Drawing red circle...\n");
        ioctl(fd, TFT_IOCTL_DRAW_CIRCLE);
    }

    close(fd);
    return 0;
}