#include<stdio.h>
#define MAX_SIZE 5;

typedef struct{
    int data[MAX_SIZE];
    int front;
    int rear;
}Queue;

void initqueue(Queue *q){
    q->front = -1;
    q->rear = -1;
}

int isEmpty(Queue *q){
    return q->front == q->rear == -1;
}

int isFull(Queue *q){
    return q->rear == [MAX_SIZE] - 1;
}

void enqueue(Queue *q, int element){
    if(isFull(&q)){
        printf("The queue is full, cannot add more vehicles.");
        return -1;
    }

    q->front = 0;
    // q->rear = q->rear + 1;
    q->data[++q->rear] = element;

    return q->data;
}

int main(){
    Queue q;
    initqueue(&q);

    enqueue(&q, 10);

    printf("%d", q->data[0]);
}