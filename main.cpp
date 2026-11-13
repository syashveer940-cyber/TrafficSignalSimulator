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
        else if(t==TURN_RIGHT){i.p1={c.x-70,c.y+70}; i.p2={c.x-70,c.y+140}; i.exitDir={0,1};}
        else if(t==TURN_LEFT){i.p1={c.x+70,c.y+70}; i.p2={c.x+70,c.y-140}; i.exitDir={0,-1};}
        else{i.p1={c.x+40,c.y}; i.p2={c.x-140,c.y-70}; i.exitDir={-1,0};}
    } return i;
}
Vector2 computeVehiclePosition(int l, TurnType t, float p, Vector2 c) {
    if(p>0.0f) { if(l==0)return{c.x-70,c.y-140-p}; if(l==1)return{c.x+70,c.y+140+p}; if(l==2)return{c.x+140+p,c.y-70}; return{c.x-140-p,c.y+70}; }
    PathInfo i=getPathInfo(l,t,c);
    if(-p<=280.0f) { float u=-p/280.0f, v=1.0f-u; return {v*v*i.p0.x+2.0f*v*u*i.p1.x+u*u*i.p2.x, v*v*i.p0.y+2.0f*v*u*i.p1.y+u*u*i.p2.y}; }
    return {i.p2.x+i.exitDir.x*(-p-280.0f), i.p2.y+i.exitDir.y*(-p-280.0f)};
}
Vector2 getLaneDirection(int l, TurnType t, float p, Vector2 c) {
    Vector2 pA=computeVehiclePosition(l,t,p,c), pB=computeVehiclePosition(l,t,p-2.0f,c);
    float len=hypot(pB.x-pA.x,pB.y-pA.y); return len==0.0f?Vector2{0,1}:Vector2{(pB.x-pA.x)/len,(pB.y-pA.y)/len};
}
int getExitDir(int l, TurnType t) {
    if(l==0) return t==STRAIGHT?1:t==TURN_RIGHT?3:t==TURN_LEFT?2:0; if(l==1) return t==STRAIGHT?0:t==TURN_RIGHT?2:t==TURN_LEFT?3:1;
    if(l==2) return t==STRAIGHT?3:t==TURN_RIGHT?0:t==TURN_LEFT?1:2; return t==STRAIGHT?2:t==TURN_RIGHT?1:t==TURN_LEFT?0:3;
}
TurnType getTurnTypeForNeighbor(int l, int e) {
    if(l==0) return e==1?STRAIGHT:e==3?TURN_RIGHT:e==2?TURN_LEFT:U_TURN; if(l==1) return e==0?STRAIGHT:e==2?TURN_RIGHT:e==3?TURN_LEFT:U_TURN;
    if(l==2) return e==3?STRAIGHT:e==0?TURN_RIGHT:e==1?TURN_LEFT:U_TURN; return e==2?STRAIGHT:e==1?TURN_RIGHT:e==0?TURN_LEFT:U_TURN;
}
float getSafeYieldMultiplier(int l, int id, TurnType t, float pos, const std::vector<CarRect>& cars, Vector2 c) {
    Vector2 curP = computeVehiclePosition(l, t, pos, c);
    for (float step=8.0f; step<=75.0f; step+=10.0f) {
        Vector2 futP = computeVehiclePosition(l, t, pos-step, c);
        for (const auto& o : cars) {
            if (o.id == id) continue;
            float cD = hypot(curP.x-(o.rect.x+20), curP.y-(o.rect.y+20)), fD = hypot(futP.x-(o.rect.x+20), futP.y-(o.rect.y+20));
            if (fD < 50.0f && fD <= cD) return 0.0f;
            if (cD < 48.0f && (o.laneIndex == l ? pos > o.pos : o.id < id)) return 0.0f;
        }
    } return 1.0f;
}

class TrafficLane {
private: 
    LightState state; int bX, bY; Vector2 center;
    void drawVehicle(Vector2 p, Vector2 dir, bool emg, bool reck, TimeOfDay tod) {
        DrawRectangle(p.x-20, p.y-20, 40, 40, emg?RED:(reck?MAGENTA:BLUE)); DrawRectangleLines(p.x-20, p.y-20, 40, 40, BLACK);
        if (tod==TIME_NIGHT||tod==TIME_EVENING) {
            Vector2 pR={-dir.y,dir.x}; DrawCircleV({p.x+dir.x*20+pR.x*11,p.y+dir.y*20+pR.y*11},3.2f,Color{255,255,220,255});
