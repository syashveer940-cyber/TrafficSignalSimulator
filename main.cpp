#include "raylib.h"
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <queue>

using namespace std;

enum LightState { RED_STATE, YELLOW_STATE, GREEN_STATE };
enum TurnType { STRAIGHT, TURN_RIGHT, TURN_LEFT, U_TURN };

struct Vehicle { int id, isEmergency; bool isReckless; int targetDest; float pos; Vehicle* next; int uTurnCount; };
struct Queue { Vehicle *front = nullptr, *rear = nullptr; int totalCount = 0, emergencyCount = 0; };

Queue* createQueue() { return new Queue(); }

void enqueue(Queue* q, int id, int isEmergency, bool isReckless, int targetDest, float startPos) {
    Vehicle* v = new Vehicle(); v->id = id; v->isEmergency = isEmergency; v->isReckless = isReckless; v->targetDest = targetDest; v->next = nullptr; v->uTurnCount = 0;
    float actualPos = startPos;
    // Strict spawn spacing to prevent overlaps NO MATTER WHAT
    if (q->rear != nullptr && (q->rear->pos + 60.0f) > actualPos) actualPos = q->rear->pos + 60.0f;
    v->pos = actualPos;
    if (q->rear == nullptr) q->front = q->rear = v; else { q->rear->next = v; q->rear = v; }
    q->totalCount++; if (isEmergency) q->emergencyCount++;
}

Vehicle* dequeue(Queue* q) {
    if (q->front == nullptr) return nullptr;
    Vehicle* temp = q->front; q->front = q->front->next;
    if (q->front == nullptr) q->rear = nullptr;
    q->totalCount--; if (temp->isEmergency) q->emergencyCount--; return temp;
}

void removeVehicleById(Queue* q, int id) {
    Vehicle *current = q->front, *prev = nullptr;
    while (current != nullptr) {
        if (current->id == id) {
            if (prev == nullptr) q->front = current->next; else prev->next = current->next;
            if (current == q->rear) q->rear = prev;
            q->totalCount--; if (current->isEmergency) q->emergencyCount--; delete current; return;
        }
        prev = current; current = current->next;
    }
}

void insertIntoQueueSorted(Queue* q, int id, int isEmergency, bool isReckless, int targetDest, float pos, int uTurnCount = 0) {
    Vehicle* v = new Vehicle(); v->id = id; v->isEmergency = isEmergency; v->isReckless = isReckless; v->targetDest = targetDest; v->pos = pos; v->next = nullptr; v->uTurnCount = uTurnCount;
    q->totalCount++; if (isEmergency) q->emergencyCount++;
    if (q->front == nullptr) { q->front = q->rear = v; return; }
    if (pos < q->front->pos) { v->next = q->front; q->front = v; return; }
    Vehicle* current = q->front;
    while (current->next != nullptr && current->next->pos <= pos) current = current->next;
    v->next = current->next; current->next = v; if (v->next == nullptr) q->rear = v;
}

void destroyQueue(Queue* q) { while (q->front != nullptr) delete dequeue(q); delete q; }

struct ExitingVehicle { int id; bool isEmergency, isReckless; int targetDest; TurnType turn; float pos; int uTurnCount; };
struct UTurningVehicle { int id; bool isEmergency, isReckless; int targetDest; TurnType originTurn; float startPos, destPos; int destLaneIdx; bool destIsQueue; Vector2 p0, p1, p2, p3; float progress; int uTurnCount; };
