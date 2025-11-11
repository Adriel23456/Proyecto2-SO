#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int num_bars = 20;
    int i;
    FILE *fp;
    
    if (argc > 1) {
        num_bars = atoi(argv[1]);
        if (num_bars < 1 || num_bars > 256) {
            fprintf(stderr, "Number of bars must be between 1 and 256\n");
            return 1;
        }
    }
    
    fp = fopen("histogram_data.dat", "w");
    if (!fp) {
        perror("Failed to create file");
        return 1;
    }
    
    srand(time(NULL));
    
    for (i = 0; i < num_bars; i++) {
        int value = 50 + rand() % 200;
        fprintf(fp, "%d\n", value);
    }
    
    fclose(fp);
    printf("Generated histogram_data.dat with %d bars\n", num_bars);
    
    return 0;
}