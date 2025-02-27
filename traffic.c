#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define VEHICLE_FILE_A "lanea.txt"
#define VEHICLE_FILE_B "laneb.txt"
#define VEHICLE_FILE_C "lanec.txt"
#define VEHICLE_FILE_D "laned.txt"

// Function to generate a random vehicle ID
void generateVehicle(char *id, char *lane) {
    sprintf(id, "V%d", rand() % 1000);
    char lanes[] = {'A', 'B', 'C', 'D'};
    *lane = lanes[rand() % 4];
}

// Function to write vehicle to appropriate lane file
void writeVehicleToFile(char *vehicleId, char lane) {
    FILE *file;
    switch (lane) {
        case 'A':
            file = fopen(VEHICLE_FILE_A, "a");
            break;
        case 'B':
            file = fopen(VEHICLE_FILE_B, "a");
            break;
        case 'C':
            file = fopen(VEHICLE_FILE_C, "a");
            break;
        case 'D':
            file = fopen(VEHICLE_FILE_D, "a");
            break;
        default:
            return;
    }
    fprintf(file, "%s:%c\n", vehicleId, lane);
    fclose(file);
}

int main() {
    srand(time(NULL));
    char vehicleId[20];
    char lane;
    
    for (int i = 0; i < 20; i++) {
        generateVehicle(vehicleId, &lane);
        printf("Generated Vehicle %s in Lane %c\n", vehicleId, lane);
        writeVehicleToFile(vehicleId, lane);
    }

    return 0;
}
