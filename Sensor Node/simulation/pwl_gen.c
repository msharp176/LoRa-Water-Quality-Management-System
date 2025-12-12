#include <stdio.h>
#include <stdlib.h>
#include <math.h>

double turbiditySensor(int time_ms) {
    return (0.0000005 * powf(time_ms, 2)) - (0.0018 * time_ms) + 3.6679; 
}

int main(void) {

    char * filename = "C:\\Users\\mshar\\Documents\\capstone\\simulation\\turb_sensor.txt";

    FILE * fptr = fopen(filename, "w+");

    if (fptr == NULL) {
        printf("Failed to open %s!\n", filename);
        return 0;
    }

    for (int k = 0; k <= 1500; k++) {
        fprintf(fptr, "%dm %f", k, turbiditySensor(k));
        if (k != 1500) {
            fprintf(fptr, "\n");
        }
    }

    fclose(fptr);

    printf("Successfully wrote %s.\n", filename);

}