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
#define PRIORITY_THRESHOLD 10
#define PRIORITY_RESET_THRESHOLD 5
#define VEHICLE_PASS_TIME 1
#define NUM_SUB_LANES 3 // Number of sub-lanes per main lane

// Lane file names
const char *LANE_FILES[] = {"lanea.txt", "laneb.txt", "lanec.txt", "laned.txt"};
const char *LANE_NAMES[] = {"A", "B", "C", "D"};

// Vehicle structure
typedef struct Vehicle
{
    char id[20];
    struct Vehicle *next;
} Vehicle;

// Queue structure
typedef struct VehicleQueue
{
    Vehicle *front;
    Vehicle *rear;
    int count;
    Vehicle *front[NUM_SUB_LANES]; // Front of queue for each sub-lane
    Vehicle *rear[NUM_SUB_LANES];  // Rear of queue for each sub-lane
    int count[NUM_SUB_LANES];      // Count of vehicles in each sub-lane
} VehicleQueue;

// SharedData structure
typedef struct
{
    VehicleQueue *laneQueues[4]; // A, B, C, D
    int currentServingLane;
    int timeLeft;
    bool priorityMode;
} SharedData;

// Function declarations
void enqueue(VehicleQueue *queue, const char *vehicleId);
Vehicle *dequeue(VehicleQueue *queue);
void *processQueues(void *arg);
void *monitorLaneFiles(void *arg);
void drawRoadsAndLights(SDL_Renderer *renderer, SharedData *sharedData, TTF_Font *font);
void drawLight(SDL_Renderer *renderer, int laneIndex, bool isRed);
void displayText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y);

int main()
{
    srand(time(NULL));

    // Initialize SDL and font
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *window = SDL_CreateWindow("Traffic Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/dejavu/DejaVuSans.ttf", 24);

    if (!font)
    {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        return -1;
    }

    // Initialize shared data and vehicle queues
    SharedData sharedData = {0};
    for (int i = 0; i < 4; i++)
    {
        sharedData.laneQueues[i] = (VehicleQueue *)malloc(sizeof(VehicleQueue));
        sharedData.laneQueues[i]->front = NULL;
        sharedData.laneQueues[i]->rear = NULL;
        sharedData.laneQueues[i]->count = 0;
    }
    sharedData.currentServingLane = -1;
    sharedData.timeLeft = 0;
    sharedData.priorityMode = false;

    // Create threads for queue processing and lane monitoring
    pthread_t tQueue, tMonitor;
    pthread_create(&tQueue, NULL, processQueues, &sharedData);
    pthread_create(&tMonitor, NULL, monitorLaneFiles, &sharedData);

    // SDL rendering loop
    SDL_Event event;
    bool running = true;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Clear the screen with white background
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Draw roads, lights, and vehicle queues
        drawRoadsAndLights(renderer, &sharedData, font);

        // Present the rendered frame
        SDL_RenderPresent(renderer);

        // Delay to control frame rate
        SDL_Delay(100);
    }

    // Cleanup
    TTF_Quit();
    SDL_Quit();
    return 0;
}

// Enqueue a vehicle into the specified sub-lane
void enqueue(VehicleQueue *queue, const char *vehicleId, int subLane)
{
    Vehicle *newVehicle = (Vehicle *)malloc(sizeof(Vehicle));
    strcpy(newVehicle->id, vehicleId);
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

// Dequeue a vehicle from the specified sub-lane
Vehicle *dequeue(VehicleQueue *queue, int subLane)
{
    if (queue->front[subLane] == NULL)
        return NULL;

    Vehicle *temp = queue->front[subLane];
    queue->front[subLane] = queue->front[subLane]->next;

    if (queue->front[subLane] == NULL)
    {
        queue->rear[subLane] = NULL;
    }

    queue->count[subLane]--;
    return temp;
}

// Function to monitor lane files
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
                    enqueue(sharedData->laneQueues[i], vehicleId);
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
            sharedData->priorityMode = sharedData->laneQueues[0]->count >= PRIORITY_RESET_THRESHOLD;
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                if (sharedData->laneQueues[i]->count > 0)
                {
                    servedLane = i;
                    break;
                }
            }
        }

        if (servedLane != -1)
        {
            sharedData->currentServingLane = servedLane;
            Vehicle *vehicle = dequeue(sharedData->laneQueues[servedLane]);
            if (vehicle)
            {
                printf("Serving vehicle %s from lane %s\n", vehicle->id, LANE_NAMES[servedLane]);
                free(vehicle);
                sleep(VEHICLE_PASS_TIME);
            }
        }
        else
        {
            sharedData->currentServingLane = -1;
        }

        if (sharedData->laneQueues[0]->count >= PRIORITY_THRESHOLD)
        {
            sharedData->priorityMode = true;
        }

        sleep(1);
    }
}

void drawVehiclesInLane(SDL_Renderer *renderer, VehicleQueue *queue, int laneIndex)
{
    int x, y;

    // Set vehicle color
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for vehicles

    Vehicle *current = queue->front;
    int vehicleOffset = 20; // Spacing between vehicles

    // Loop through the vehicles in the lane and draw them
    for (int i = 0; current != NULL; i++, current = current->next)
    {
        switch (laneIndex)
        {
        case 0: // Lane A (Top -> Bottom)
            x = WINDOW_WIDTH / 2 - ROAD_WIDTH / 4;
            y = 50 + i * vehicleOffset;
            break;
        case 1: // Lane B (Bottom -> Top)
            x = WINDOW_WIDTH / 2 + ROAD_WIDTH / 4;
            y = WINDOW_HEIGHT - 50 - i * vehicleOffset;
            break;
        case 2: // Lane C (Right -> Left)
            x = WINDOW_WIDTH - 50 - i * vehicleOffset;
            y = WINDOW_HEIGHT / 2 + ROAD_WIDTH / 4;
            break;
        case 3: // Lane D (Left -> Right)
            x = 50 + i * vehicleOffset;
            y = WINDOW_HEIGHT / 2 - ROAD_WIDTH / 4;
            break;
        default:
            return;
        }

        // Draw a rectangle representing the vehicle
        SDL_Rect vehicleRect = {x, y, 20, 10}; // Each vehicle is a small rectangle
        SDL_RenderFillRect(renderer, &vehicleRect);
    }
}

void drawRoadsAndLights(SDL_Renderer *renderer, SharedData *sharedData, TTF_Font *font)
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
        displayText(renderer, font, countText, (i % 2) * 700 + 50, (i / 2) * 700 + 50);
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

// Function to display text
void displayText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y)
{
    SDL_Color textColor = {0, 0, 0, 255}; // Black color for text
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    SDL_Rect textRect = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_DestroyTexture(texture);
}
