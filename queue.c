#include "queue.h"
#include <stdlib.h>

Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->totalCount = 0;
    q->emergencyCount = 0;
    return q;
}

void enqueue(Queue* q, int id, int isEmergency) {
    Vehicle* v = (Vehicle*)malloc(sizeof(Vehicle));
    v->id = id;
    v->isEmergency = isEmergency;
    v->next = NULL;

    
    float spawnPos = 350.0f;
    if (q->rear != NULL && (q->rear->pos + 50.0f) > spawnPos) {
        spawnPos = q->rear->pos + 50.0f;
    }
    v->pos = spawnPos;

    if (q->rear == NULL) {
        q->front = q->rear = v;
    } else {
        q->rear->next = v;
        q->rear = v;
    }
    q->totalCount++;
    if (isEmergency) q->emergencyCount++;
}

Vehicle* dequeue(Queue* q) {
    if (q->front == NULL) return NULL;
    
    Vehicle* temp = q->front;
    q->front = q->front->next;
    
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    q->totalCount--;
    if (temp->isEmergency) q->emergencyCount--;
    
    return temp;
}

void destroyQueue(Queue* q) {
    while (q->front != NULL) {
        Vehicle* temp = dequeue(q);
        free(temp);
    }
    free(q);
}
