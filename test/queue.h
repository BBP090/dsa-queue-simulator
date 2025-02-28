#ifndef QUEUE_H
#define QUEUE_H

typedef struct Vehicle {
    char id[20];
    int x, y;
    int laneIndex;
    int subLaneIndex;
} Vehicle;

typedef struct Node {
    Vehicle vehicle;
    struct Node* next;
} Node;

typedef struct VehicleQueue {
    Node* front;
    Node* rear;
    int size;
} VehicleQueue;

void initQueue(VehicleQueue* queue);
void enqueue(VehicleQueue* queue, Vehicle* vehicle);
Vehicle dequeue(VehicleQueue* queue);

#endif
