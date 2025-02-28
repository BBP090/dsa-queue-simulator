#include "queue.h"
#include <stdlib.h>

void initQueue(VehicleQueue* queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

void enqueue(VehicleQueue* queue, Vehicle* vehicle) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->vehicle = *vehicle;
    newNode->next = NULL;

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }

    queue->size++;
}

Vehicle dequeue(VehicleQueue* queue) {
    if (queue->front == NULL) return (Vehicle){};

    Node* temp = queue->front;
    Vehicle vehicle = temp->vehicle;
    queue->front = queue->front->next;
    if (queue->front == NULL) queue->rear = NULL;

    free(temp);
    queue->size--;
    return vehicle;
}
