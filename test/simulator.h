#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <SDL2/SDL.h>
#include "queue.h"

void run_simulation(SDL_Renderer* renderer);
void drawRoads(SDL_Renderer* renderer);
void drawVehicles(SDL_Renderer* renderer, VehicleQueue* lanes);

#endif
