#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vehicle {
    int id;
    int isEmergency;
    float pos; 
    struct Vehicle* next;
} Vehicle;

typedef struct Queue {
    Vehicle* front;
    Vehicle* rear;
    int totalCount;
    int emergencyCount;
} Queue;

Queue* createQueue();
void enqueue(Queue* q, int id, int isEmergency);
Vehicle* dequeue(Queue* q);
void destroyQueue(Queue* q);

#ifdef __cplusplus
}
#endif

#endif
