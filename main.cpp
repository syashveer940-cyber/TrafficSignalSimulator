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
    Vehicle* v=new Vehicle(); v->id=id; v->isEmergency=isEmergency; v->isReckless=isReckless; v->targetDest=targetDest; v->pos=pos; v->next=nullptr; v->uTurnCount=uTurnCount; v->originLane=originLane;
    q->totalCount++; if (isEmergency) q->emergencyCount++;
    if (q->front==nullptr) { q->front = q->rear = v; return; }
    if (pos<q->front->pos) { v->next = q->front; q->front = v; return; }
    Vehicle* cur = q->front; while (cur->next!=nullptr && cur->next->pos<=pos) cur = cur->next;
    v->next = cur->next; cur->next = v; if (v->next==nullptr) q->rear = v;
}
void destroyQueue(Queue* q) { while (q->front!=nullptr) delete dequeue(q); delete q; }

struct ExitingVehicle { int id; bool isEmergency, isReckless; int targetDest; TurnType turn; float pos; int uTurnCount, originLane; };
struct UTurningVehicle { int id; bool isEmergency, isReckless; int targetDest; TurnType originTurn; float startPos, destPos; int destLaneIdx; bool destIsQueue; Vector2 p0,p1,p2,p3; float progress; int uTurnCount, originLane; };
struct CarRect { int id, laneIndex; bool isReckless; float pos; Rectangle rect; };
struct PathInfo { Vector2 p0, p1, p2, exitDir; };
struct Skyscraper { Rectangle base; float height; Color col; };
struct CrashEvent { Vector2 pos; int framesLeft, car1Id, car2Id; };
struct UpcomingEntryLane { int intId, laneIdx; const char *label, *fullName; Color color; int maxCapacity, targetPer30s; float spawnTimer; bool isUserCustomized; };
class Intersection;

Color AdjustBrightness(Color c, float f) { return Color{(unsigned char)max(0,min(c.r+(int)(255*f),255)),(unsigned char)max(0,min(c.g+(int)(255*f),255)),(unsigned char)max(0,min(c.b+(int)(255*f),255)),255}; }

PathInfo getPathInfo(int l, TurnType t, Vector2 c) {
    PathInfo i;
    if (l==0) {
        i.p0={c.x-70.0f, c.y-140.0f};
        if(t==STRAIGHT){i.p1={c.x-70,c.y}; i.p2={c.x-70,c.y+140}; i.exitDir={0,1};}
        else if(t==TURN_RIGHT){i.p1={c.x-70,c.y-70}; i.p2={c.x-140,c.y-70}; i.exitDir={-1,0};}
        else if(t==TURN_LEFT){i.p1={c.x-70,c.y+70}; i.p2={c.x+140,c.y+70}; i.exitDir={1,0};}
        else{i.p1={c.x,c.y+40}; i.p2={c.x+70,c.y-140}; i.exitDir={0,-1};}
    } else if (l==1) {
        i.p0={c.x+70.0f, c.y+140.0f};
        if(t==STRAIGHT){i.p1={c.x+70,c.y}; i.p2={c.x+70,c.y-140}; i.exitDir={0,-1};}
        else if(t==TURN_RIGHT){i.p1={c.x+70,c.y+70}; i.p2={c.x+140,c.y+70}; i.exitDir={1,0};}
        else if(t==TURN_LEFT){i.p1={c.x+70,c.y-70}; i.p2={c.x-140,c.y-70}; i.exitDir={-1,0};}
        else{i.p1={c.x,c.y-40}; i.p2={c.x-70,c.y+140}; i.exitDir={0,1};}
    } else if (l==2) {
        i.p0={c.x+140.0f, c.y-70.0f};
        if(t==STRAIGHT){i.p1={c.x,c.y-70}; i.p2={c.x-140,c.y-70}; i.exitDir={-1,0};}
        else if(t==TURN_RIGHT){i.p1={c.x+70,c.y-70}; i.p2={c.x+70,c.y-140}; i.exitDir={0,-1};}
        else if(t==TURN_LEFT){i.p1={c.x-70,c.y-70}; i.p2={c.x-70,c.y+140}; i.exitDir={0,1};}
        else{i.p1={c.x-40,c.y}; i.p2={c.x+140,c.y+70}; i.exitDir={1,0};}
    } else {
        i.p0={c.x-140.0f, c.y+70.0f};
        if(t==STRAIGHT){i.p1={c.x,c.y+70}; i.p2={c.x+140,c.y+70}; i.exitDir={1,0};}
