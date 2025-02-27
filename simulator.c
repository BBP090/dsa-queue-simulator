#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 20
#define MAIN_FONT "/usr/share/fonts/dejavu/DejaVuSans.ttf"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define SCALE 1
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define ARROW_SIZE 15

const char *VEHICLE_FILE = "vehicles.data";

typedef struct
{
    int x, y;
    int lane;
    bool active;
} Vehicle;

typedef struct
{
    int currentLight[4]; // 0: Red, 1: Green
    int nextLight[4];
} SharedData;

// Function declarations
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font);
void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y);
void drawLight(SDL_Renderer *renderer, int lane, bool isRed);
void refreshLight(SDL_Renderer *renderer, SharedData *sharedData);
void *chequeQueue(void *arg);
void *readAndParseFile(void *arg);

void printMessageHelper(const char *message, int count)
{
    for (int i = 0; i < count; i++)
        printf("%s\n", message);
}

int main()
{
    pthread_t tQueue, tReadFile;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Event event;

    if (!initializeSDL(&window, &renderer))
    {
        return -1;
    }
    SDL_mutex *mutex = SDL_CreateMutex();
    SharedData sharedData = {{0, 0, 0, 0}, {0, 0, 0, 0}}; // All lights red

    TTF_Font *font = TTF_OpenFont(MAIN_FONT, 24);
    if (!font) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    drawRoadsAndLane(renderer, font);
    SDL_RenderPresent(renderer);

    pthread_create(&tQueue, NULL, chequeQueue, &sharedData);
    pthread_create(&tReadFile, NULL, readAndParseFile, NULL);

    // Continue the UI thread
    bool running = true;
    while (running)
    {
        // update light
        refreshLight(renderer, &sharedData);
        while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                running = false;
    }
    SDL_DestroyMutex(mutex);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    // font init
    if (TTF_Init() < 0)
    {
        SDL_Log("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        return false;
    }

    *window = SDL_CreateWindow("Junction Diagram",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH * SCALE, WINDOW_HEIGHT * SCALE,
                               SDL_WINDOW_SHOWN);
    if (!*window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Use SDL_RENDERER_PRESENTVSYNC to enable V-Sync
    *renderer = SDL_CreateRenderer(*window, 2, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // if you have high resolution monitor 2K or 4K then scale
    SDL_RenderSetScale(*renderer, SCALE, SCALE);

    if (!*renderer)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}

void drawArrwow(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int x3, int y3)
{
    // Sort vertices by ascending Y (bubble sort approach)
    if (y1 > y2)
    {
        int temp;
        temp = y1;
        y1 = y2;
        y2 = temp;
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y3)
    {
        int temp;
        temp = y1;
        y1 = y3;
        y3 = temp;
        temp = x1;
        x1 = x3;
        x3 = temp;
    }
    if (y2 > y3)
    {
        int temp;
        temp = y2;
        y2 = y3;
        y3 = temp;
        temp = x2;
        x2 = x3;
        x3 = temp;
    }

    float dx1 = (y2 - y1) ? (float)(x2 - x1) / (y2 - y1) : 0;
    float dx2 = (y3 - y1) ? (float)(x3 - x1) / (y3 - y1) : 0;
    float dx3 = (y3 - y2) ? (float)(x3 - x2) / (y3 - y2) : 0;

    float sx1 = x1, sx2 = x1;

    for (int y = y1; y < y2; y++)
    {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx1;
        sx2 += dx2;
    }

    sx1 = x2;

    for (int y = y2; y <= y3; y++)
    {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx3;
        sx2 += dx2;
    }
}

void drawLight(SDL_Renderer *renderer, int lane, bool isRed)
{
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_Rect lightBox;
    SDL_Rect light;
    int arrowX, arrowY;

    switch (lane)
    {
    case 0: // North
        lightBox = (SDL_Rect){WINDOW_WIDTH / 2 - 25, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 - 50, 50, 30};
        light = (SDL_Rect){WINDOW_WIDTH / 2 - 20, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 - 45, 20, 20};
        arrowX = WINDOW_WIDTH / 2 + 10;
        arrowY = WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 - 45 + 10;
        break;
    case 1: // South
        lightBox = (SDL_Rect){WINDOW_WIDTH / 2 - 25, WINDOW_HEIGHT / 2 + ROAD_WIDTH / 2 + 20, 50, 30};
        light = (SDL_Rect){WINDOW_WIDTH / 2 - 20, WINDOW_HEIGHT / 2 + ROAD_WIDTH / 2 + 25, 20, 20};
        arrowX = WINDOW_WIDTH / 2 + 10;
        arrowY = WINDOW_HEIGHT / 2 + ROAD_WIDTH / 2 + 25 + 10;
        break;
    case 2: // West
        lightBox = (SDL_Rect){WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 - 15, 50, 30};
        light = (SDL_Rect){WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 - 45, WINDOW_HEIGHT / 2 - 10, 20, 20};
        arrowX = WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 - 45 + 10;
        arrowY = WINDOW_HEIGHT / 2 + 10;
        break;
    case 3: // East
        lightBox = (SDL_Rect){WINDOW_WIDTH / 2 + ROAD_WIDTH / 2 + 20, WINDOW_HEIGHT / 2 - 15, 50, 30};
        light = (SDL_Rect){WINDOW_WIDTH / 2 + ROAD_WIDTH / 2 + 25, WINDOW_HEIGHT / 2 - 10, 20, 20};
        arrowX = WINDOW_WIDTH / 2 + ROAD_WIDTH / 2 + 25 + 10;
        arrowY = WINDOW_HEIGHT / 2 + 10;
        break;
    }

    SDL_RenderFillRect(renderer, &lightBox);
    SDL_SetRenderDrawColor(renderer, isRed ? 255 : 11, isRed ? 0 : 156, isRed ? 0 : 50, 255);
    SDL_RenderFillRect(renderer, &light);
    drawArrwow(renderer, arrowX, arrowY, arrowX, arrowY + 20, arrowX + 10, arrowY + 10);
}

void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255);
    SDL_Rect verticalRoad = {WINDOW_WIDTH / 2 - ROAD_WIDTH / 2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &verticalRoad);
    SDL_Rect horizontalRoad = {0, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &horizontalRoad);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i <= 3; i++)
    {
        SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, WINDOW_WIDTH / 2 - ROAD_WIDTH / 2, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i);
        SDL_RenderDrawLine(renderer, 800, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, WINDOW_WIDTH / 2 + ROAD_WIDTH / 2, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i);
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, 0, WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, WINDOW_HEIGHT / 2 - ROAD_WIDTH / 2);
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, 800, WINDOW_WIDTH / 2 - ROAD_WIDTH / 2 + LANE_WIDTH * i, WINDOW_HEIGHT / 2 + ROAD_WIDTH / 2);
    }

    displayText(renderer, font, "A", 400, 10);
    displayText(renderer, font, "B", 400, 770);
    displayText(renderer, font, "D", 10, 400);
    displayText(renderer, font, "C", 770, 400);
}

void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y)
{
    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    SDL_Rect textRect = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_DestroyTexture(texture);
}

void refreshLight(SDL_Renderer *renderer, SharedData *sharedData)
{
    for (int i = 0; i < 4; i++)
    {
        if (sharedData->nextLight[i] != sharedData->currentLight[i])
        {
            drawLight(renderer, i, sharedData->nextLight[i] == 0);
            sharedData->currentLight[i] = sharedData->nextLight[i];
        }
    }
    SDL_RenderPresent(renderer);
}

void *chequeQueue(void *arg)
{
    SharedData *sharedData = (SharedData *)arg;
    while (1)
    {
        for (int i = 0; i < 4; i++)
            sharedData->nextLight[i] = 0;
        sleep(5);
        for (int i = 0; i < 4; i++)
            sharedData->nextLight[i] = 1;
        sleep(5);
    }
}

void *readAndParseFile(void *arg)
{
    while (1)
    {
        FILE *file = fopen(VEHICLE_FILE, "r");
        if (!file)
        {
            perror("Error opening file");
            continue;
        }

        char line[MAX_LINE_LENGTH];
        while (fgets(line, sizeof(line), file))
        {
            line[strcspn(line, "\n")] = 0;
            char *vehicleNumber = strtok(line, ":");
            char *road = strtok(NULL, ":");

            if (vehicleNumber && road)
                printf("Vehicle: %s, Road: %s\n", vehicleNumber, road);
            else
                printf("Invalid format: %s\n", line);
        }
        fclose(file);
        sleep(2);
    }
}