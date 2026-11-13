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
enum TimeOfDay { TIME_MORNING=0, TIME_AFTERNOON=1, TIME_EVENING=2, TIME_NIGHT=3 };
enum WeatherType { WEATHER_SUMMER=0, WEATHER_WINTER=1, WEATHER_RAINY=2, WEATHER_SPRING=3, WEATHER_AUTUMN=4 };
struct WeatherParticle { float x, y, speedX, speedY, size; };
struct Vehicle { int id, isEmergency; bool isReckless; int targetDest; float pos; Vehicle* next; int uTurnCount, originLane; };
struct Queue { Vehicle *front=nullptr, *rear=nullptr; int totalCount=0, emergencyCount=0; };
Queue* createQueue() { return new Queue(); }

void enqueue(Queue* q, int id, int isEmergency, bool isReckless, int targetDest, float startPos, float minSpacing=60.0f, int originLane=0) {
    Vehicle* v = new Vehicle(); v->id=id; v->isEmergency=isEmergency; v->isReckless=isReckless; v->targetDest=targetDest; v->next=nullptr; v->uTurnCount=0; v->originLane=originLane;
    float actualPos = startPos; if (q->rear != nullptr && (q->rear->pos + minSpacing) > actualPos) actualPos = q->rear->pos + minSpacing;
    v->pos = actualPos; if (q->rear == nullptr) q->front = q->rear = v; else { q->rear->next = v; q->rear = v; }
    q->totalCount++; if (isEmergency) q->emergencyCount++;
}
Vehicle* dequeue(Queue* q) {
    if (q->front == nullptr) return nullptr;
    Vehicle* temp = q->front; q->front = q->front->next; if (q->front == nullptr) q->rear = nullptr;
    q->totalCount--; if (temp->isEmergency) q->emergencyCount--; return temp;
}
void removeVehicleById(Queue* q, int id) {
    Vehicle *cur=q->front, *prev=nullptr;
    while (cur!=nullptr) {
        if (cur->id==id) {
            if (prev==nullptr) q->front = cur->next; else prev->next = cur->next;
            if (cur==q->rear) q->rear = prev;
            q->totalCount--; if (cur->isEmergency) q->emergencyCount--; delete cur; return;
        }
        prev=cur; cur=cur->next;
    }
}
void insertIntoQueueSorted(Queue* q, int id, int isEmergency, bool isReckless, int targetDest, float pos, int uTurnCount=0, int originLane=0) {
