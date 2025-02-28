#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 20
#define VEHICLE_HEIGHT 10
#define VEHICLE_SPEED 5
#define NUM_SUB_LANES 3
#define PRIORITY_THRESHOLD 10
#define PRIORITY_RESET_THRESHOLD 5
#define VEHICLE_PASS_TIME 1

// Lane file names
const char *LANE_FILES[] = {"lanea.txt", "laneb.txt", "lanec.txt", "laned.txt"};
const char *LANE_NAMES[] = {"A", "B", "C", "D"};

// Vehicle structure
typedef struct Vehicle
{
    char id[20];
    int x, y;
    int laneIndex;
    int subLaneIndex;
    int direction;
    struct Vehicle *next;
} Vehicle;

// Queue structure with 3 sub-lanes per main lane
typedef struct VehicleQueue
{
    Vehicle *front[NUM_SUB_LANES];
    Vehicle *rear[NUM_SUB_LANES];
    int count[NUM_SUB_LANES];
} VehicleQueue;

// SharedData structure
typedef struct
{
    VehicleQueue *laneQueues[4];
    int currentServingLane;
    int timeLeft;
    bool priorityMode;
} SharedData;

// Function declarations
void enqueue(VehicleQueue *queue, const char *vehicleId, int subLane, int laneIndex);
Vehicle *dequeue(VehicleQueue *queue, int subLane);
void *processQueues(void *arg);
void *monitorLaneFiles(void *arg);
void drawRoadsAndLights(SDL_Renderer *renderer, SharedData *sharedData);
void drawLight(SDL_Renderer *renderer, int laneIndex, bool isRed);
void drawVehiclesInLane(SDL_Renderer *renderer, VehicleQueue *queue, int laneIndex);
void moveVehicles(VehicleQueue *queue);
void animateVehicles(SDL_Renderer *renderer, SharedData *sharedData);
void displayText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y);

int main()
{
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow("Traffic Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/dejavu/DejaVuSans.ttf", 24);
    if (!font)
    {
        printf("Failed to load font: %s\n", TTF_GetError());
        return -1;
    }

    SharedData sharedData = {0};
    for (int i = 0; i < 4; i++)
    {
        sharedData.laneQueues[i] = (VehicleQueue *)malloc(sizeof(VehicleQueue));
        for (int j = 0; j < NUM_SUB_LANES; j++)
        {
            sharedData.laneQueues[i]->front[j] = NULL;
            sharedData.laneQueues[i]->rear[j] = NULL;
            sharedData.laneQueues[i]->count[j] = 0;
        }
    }

    pthread_t tQueue, tMonitor;
    pthread_create(&tQueue, NULL, processQueues, &sharedData);
    pthread_create(&tMonitor, NULL, monitorLaneFiles, &sharedData);

    SDL_Event event;
    bool running = true;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        animateVehicles(renderer, &sharedData);
        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}

// Function to enqueue vehicles into a specific sub-lane
void enqueue(VehicleQueue *queue, const char *vehicleId, int subLane, int laneIndex)
{
    Vehicle *newVehicle = (Vehicle *)malloc(sizeof(Vehicle));
    strcpy(newVehicle->id, vehicleId);
    newVehicle->laneIndex = laneIndex;
    newVehicle->subLaneIndex = subLane;

    // Set starting positions based on lane and direction
    switch (laneIndex)
    {
    case 0: // Lane A (Top -> Bottom)
        newVehicle->x = WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + subLane * 40;
        newVehicle->y = 0;
        newVehicle->direction = 0;
        break;
    case 1: // Lane B (Bottom -> Top)
        newVehicle->x = WINDOW_WIDTH / 2 + ROAD_WIDTH / 2 - subLane * 40;
        newVehicle->y = WINDOW_HEIGHT;
        newVehicle->direction = 1;
        break;
    case 2: // Lane C (Right -> Left)
        newVehicle->x = WINDOW_WIDTH;
        newVehicle->y = WINDOW_HEIGHT / 2 + ROAD_WIDTH / 2 - subLane * 40;
        newVehicle->direction = 2;
        break;
    case 3: // Lane D (Left -> Right)
        newVehicle->x = 0;
        newVehicle->y = WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + subLane * 40;
        newVehicle->direction = 3;
        break;
    }
    newVehicle->next = NULL;

    if (queue->rear[subLane] == NULL)
    {
        queue->front[subLane] = newVehicle;
        queue->rear[subLane] = newVehicle;
    }
    else
    {
        queue->rear[subLane]->next = newVehicle;
        queue->rear[subLane] = newVehicle;
    }

    queue->count[subLane]++;
}

// Function to dequeue vehicles from a specific sub-lane
Vehicle *dequeue(VehicleQueue *queue, int subLane)
{
    if (queue->front[subLane] == NULL)
        return NULL;
    Vehicle *temp = queue->front[subLane];
    queue->front[subLane] = queue->front[subLane]->next;
    if (queue->front[subLane] == NULL)
        queue->rear[subLane] = NULL;
    queue->count[subLane]--;
    return temp;
}

// Function to monitor lane files and enqueue vehicles to random sub-lanes
void *monitorLaneFiles(void *arg)
{
    SharedData *sharedData = (SharedData *)arg;

    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            FILE *file = fopen(LANE_FILES[i], "r");
            if (!file)
                continue;

            char line[50];
            while (fgets(line, sizeof(line), file))
            {
                char *vehicleId = strtok(line, ":");
                if (vehicleId)
                {
                    int subLane = rand() % NUM_SUB_LANES; // Randomly assign to a sub-lane
                    enqueue(sharedData->laneQueues[i], vehicleId, subLane, i);
                }
            }

            fclose(file);
            remove(LANE_FILES[i]);
        }
        sleep(1);
    }
}

// Function to process queues and handle lights
void *processQueues(void *arg)
{
    SharedData *sharedData = (SharedData *)arg;

    while (1)
    {
        int servedLane = -1;
        if (sharedData->priorityMode)
        {
            servedLane = 0; // Always serve lane A in priority mode
            sharedData->priorityMode = sharedData->laneQueues[0]->count[0] >= PRIORITY_RESET_THRESHOLD;
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < NUM_SUB_LANES; j++)
                {
                    if (sharedData->laneQueues[i]->count[j] > 0)
                    {
                        servedLane = i;
                        break;
                    }
                }
            }
        }

        if (servedLane != -1)
        {
            sharedData->currentServingLane = servedLane;
            int subLane = rand() % NUM_SUB_LANES;
            Vehicle *vehicle = dequeue(sharedData->laneQueues[servedLane], subLane);
            if (vehicle)
            {
                printf("Serving vehicle %s from lane %s sub-lane %d\n", vehicle->id, LANE_NAMES[servedLane], subLane);
                free(vehicle);
                sleep(VEHICLE_PASS_TIME);
            }
        }
        else
        {
            sharedData->currentServingLane = -1;
        }

        if (sharedData->laneQueues[0]->count[0] >= PRIORITY_THRESHOLD)
        {
            sharedData->priorityMode = true;
        }

        sleep(1);
    }
}

// Function to move vehicles in each lane
void moveVehicles(VehicleQueue *queue)
{
    for (int subLane = 0; subLane < NUM_SUB_LANES; subLane++)
    {
        Vehicle *current = queue->front[subLane];
        while (current != NULL)
        {
            // Move based on the direction of the lane
            switch (current->direction)
            {
            case 0: // Top -> Bottom
                current->y += VEHICLE_SPEED;
                if (current->y > WINDOW_HEIGHT)
                    current->y = 0; // Loop back
                break;
            case 1: // Bottom -> Top
                current->y -= VEHICLE_SPEED;
                if (current->y < 0)
                    current->y = WINDOW_HEIGHT; // Loop back
                break;
            case 2: // Right -> Left
                current->x -= VEHICLE_SPEED;
                if (current->x < 0)
                    current->x = WINDOW_WIDTH; // Loop back
                break;
            case 3: // Left -> Right
                current->x += VEHICLE_SPEED;
                if (current->x > WINDOW_WIDTH)
                    current->x = 0; // Loop back
                break;
            }
            current = current->next;
        }
    }
}

// Function to draw vehicles in each lane with sub-lanes
void drawVehiclesInLane(SDL_Renderer *renderer, VehicleQueue *queue, int laneIndex)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for vehicles

    for (int subLane = 0; subLane < NUM_SUB_LANES; subLane++)
    {
        Vehicle *current = queue->front[subLane];
        while (current != NULL)
        {
            SDL_Rect vehicleRect = {current->x, current->y, VEHICLE_WIDTH, VEHICLE_HEIGHT};
            SDL_RenderFillRect(renderer, &vehicleRect);
            current = current->next;
        }
    }
}

// Function to animate vehicles (move and render)
void animateVehicles(SDL_Renderer *renderer, SharedData *sharedData)
{
    // Move and draw vehicles in all lanes
    for (int i = 0; i < 4; i++)
    {
        moveVehicles(sharedData->laneQueues[i]);
        drawVehiclesInLane(renderer, sharedData->laneQueues[i], i);
    }
}

void drawRoadsAndLights(SDL_Renderer *renderer, SharedData *sharedData)
{
    // Set the background color
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255); // Light gray for roads

    // Draw vertical road
    SDL_Rect verticalRoad = {WINDOW_WIDTH / 2 - ROAD_WIDTH / 2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Draw horizontal road
    SDL_Rect horizontalRoad = {0, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Set color for lane dividers
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for dividers

    // Draw lane dividers (horizontal)
    for (int i = 0; i <= 3; i++)
    {
        SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i,
                           WINDOW_WIDTH, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i);
    }

    // Draw lane dividers (vertical)
    for (int i = 0; i <= 3; i++)
    {
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, 0,
                           WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, WINDOW_HEIGHT);
    }

    // Draw traffic lights for each lane
    for (int i = 0; i < 4; i++)
    {
        bool isRed = (sharedData->currentServingLane != i); // Red if not serving the lane
        drawLight(renderer, i, isRed);
    }

    // Draw vehicles in each lane
    for (int i = 0; i < 4; i++)
    {
        drawVehiclesInLane(renderer, sharedData->laneQueues[i], i);
    }

    // Draw vehicle counts for each lane
    char countText[50];
    for (int i = 0; i < 4; i++)
    {
        sprintf(countText, "Lane %s: %d vehicles", LANE_NAMES[i], sharedData->laneQueues[i]->count);
        // displayText(renderer, , countText, (i % 2) * 700 + 50, (i / 2) * 700 + 50);
    }
}

// Function to draw traffic light for a specific lane
void drawLight(SDL_Renderer *renderer, int laneIndex, bool isRed)
{
    int x, y;
    switch (laneIndex)
    {
    case 0: // Lane A (Top)
        x = WINDOW_WIDTH / 2 - 25;
        y = WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 - 50;
        break;
    case 1: // Lane B (Bottom)
        x = WINDOW_WIDTH / 2 - 25;
        y = WINDOW_HEIGHT / 2 + ROAD_WIDTH / 2 + 10;
        break;
    case 2: // Lane C (Right)
        x = WINDOW_WIDTH / 2 + ROAD_WIDTH / 2 + 10;
        y = WINDOW_HEIGHT / 2 - 25;
        break;
    case 3: // Lane D (Left)
        x = WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 - 50;
        y = WINDOW_HEIGHT / 2 - 25;
        break;
    }

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); // Gray for light box
    SDL_Rect lightBox = {x, y, 50, 30};
    SDL_RenderFillRect(renderer, &lightBox);

    // Set the color for the traffic light
    if (isRed)
    {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
    }

    SDL_Rect light = {x + 15, y + 5, 20, 20};
    SDL_RenderFillRect(renderer, &light);
}
