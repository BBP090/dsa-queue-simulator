#include "simulator.h"
#include "queue.h"
#include <SDL2/SDL.h>
#include <stdio.h>

void run_simulation(SDL_Renderer *renderer)
{
    VehicleQueue lanes[4];
    for (int i = 0; i < 4; i++)
    {
        initQueue(&lanes[i]);
    }

    // Main simulation loop
    while (1)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Update vehicle positions, process queues, etc.
        drawRoads(renderer);
        drawVehicles(renderer, lanes);

        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }
}

void drawRoads(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255);
    SDL_Rect verticalRoad = {400 - 75, 0, 150, 600};
    SDL_RenderFillRect(renderer, &verticalRoad);

    SDL_Rect horizontalRoad = {0, 300 - 75, 800, 150};
    SDL_RenderFillRect(renderer, &horizontalRoad);
}

void drawVehicles(SDL_Renderer *renderer, VehicleQueue *lanes)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for vehicles

    for (int i = 0; i < 4; i++)
    {
        Node *current = lanes[i].front; // Use Node* here instead of Vehicle*

        while (current)
        {
            Vehicle *vehicle = &(current->vehicle); // Access the vehicle inside the node
            SDL_Rect vehicleRect = {vehicle->x, vehicle->y, 20, 10};
            SDL_RenderFillRect(renderer, &vehicleRect);

            current = current->next; // Move to the next node
        }
    }
}
