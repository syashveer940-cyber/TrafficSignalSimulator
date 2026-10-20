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
struct CarRect { int id, laneIndex; bool isReckless; float pos; Rectangle rect; };
struct PathInfo { Vector2 p0, p1, p2, exitDir; };
struct Skyscraper { Rectangle base; float height; Color col; };
struct CrashEvent { Vector2 pos; int framesLeft; int car1Id; int car2Id; };
class Intersection;

Color AdjustBrightness(Color c, float factor) { return Color{(unsigned char)max(0, min(c.r + (int)(255 * factor), 255)), (unsigned char)max(0, min(c.g + (int)(255 * factor), 255)), (unsigned char)max(0, min(c.b + (int)(255 * factor), 255)), 255}; }

PathInfo getPathInfo(int laneIdx, TurnType turn, Vector2 center) {
    PathInfo info;
    if (laneIdx == 0) {
        info.p0 = { center.x - 70.0f, center.y - 140.0f };
        if (turn == STRAIGHT) { info.p1 = { center.x - 70.0f, center.y }; info.p2 = { center.x - 70.0f, center.y + 140.0f }; info.exitDir = { 0.0f, 1.0f }; }
        else if (turn == TURN_RIGHT) { info.p1 = { center.x - 70.0f, center.y - 70.0f }; info.p2 = { center.x - 140.0f, center.y - 70.0f }; info.exitDir = { -1.0f, 0.0f }; }
        else if (turn == TURN_LEFT) { info.p1 = { center.x - 70.0f, center.y + 70.0f }; info.p2 = { center.x + 140.0f, center.y + 70.0f }; info.exitDir = { 1.0f, 0.0f }; }
        else { info.p1 = { center.x, center.y + 40.0f }; info.p2 = { center.x + 70.0f, center.y - 140.0f }; info.exitDir = { 0.0f, -1.0f }; }
    } else if (laneIdx == 1) {
        info.p0 = { center.x + 70.0f, center.y + 140.0f };
        if (turn == STRAIGHT) { info.p1 = { center.x + 70.0f, center.y }; info.p2 = { center.x + 70.0f, center.y - 140.0f }; info.exitDir = { 0.0f, -1.0f }; }
        else if (turn == TURN_RIGHT) { info.p1 = { center.x + 70.0f, center.y + 70.0f }; info.p2 = { center.x + 140.0f, center.y + 70.0f }; info.exitDir = { 1.0f, 0.0f }; }
        else if (turn == TURN_LEFT) { info.p1 = { center.x + 70.0f, center.y - 70.0f }; info.p2 = { center.x - 140.0f, center.y - 70.0f }; info.exitDir = { -1.0f, 0.0f }; }
        else { info.p1 = { center.x, center.y - 40.0f }; info.p2 = { center.x - 70.0f, center.y + 140.0f }; info.exitDir = { 0.0f, 1.0f }; }
    } else if (laneIdx == 2) {
        info.p0 = { center.x + 140.0f, center.y - 70.0f };
        if (turn == STRAIGHT) { info.p1 = { center.x, center.y - 70.0f }; info.p2 = { center.x - 140.0f, center.y - 70.0f }; info.exitDir = { -1.0f, 0.0f }; }
        else if (turn == TURN_RIGHT) { info.p1 = { center.x + 70.0f, center.y - 70.0f }; info.p2 = { center.x + 70.0f, center.y - 140.0f }; info.exitDir = { 0.0f, -1.0f }; }
        else if (turn == TURN_LEFT) { info.p1 = { center.x - 70.0f, center.y - 70.0f }; info.p2 = { center.x - 70.0f, center.y + 140.0f }; info.exitDir = { 0.0f, 1.0f }; }
        else { info.p1 = { center.x - 40.0f, center.y }; info.p2 = { center.x + 140.0f, center.y + 70.0f }; info.exitDir = { 1.0f, 0.0f }; }
    } else {
        info.p0 = { center.x - 140.0f, center.y + 70.0f };
        if (turn == STRAIGHT) { info.p1 = { center.x, center.y + 70.0f }; info.p2 = { center.x + 140.0f, center.y + 70.0f }; info.exitDir = { 1.0f, 0.0f }; }
        else if (turn == TURN_RIGHT) { info.p1 = { center.x - 70.0f, center.y + 70.0f }; info.p2 = { center.x - 70.0f, center.y + 140.0f }; info.exitDir = { 0.0f, 1.0f }; }
        else if (turn == TURN_LEFT) { info.p1 = { center.x + 70.0f, center.y + 70.0f }; info.p2 = { center.x + 70.0f, center.y - 140.0f }; info.exitDir = { 0.0f, -1.0f }; }
        else { info.p1 = { center.x + 40.0f, center.y }; info.p2 = { center.x - 140.0f, center.y - 70.0f }; info.exitDir = { -1.0f, 0.0f }; }
    }
    return info;
}

Vector2 computeVehiclePosition(int laneIdx, TurnType turn, float pos, Vector2 center) {
    if (pos > 0.0f) {
        if (laneIdx == 0) return { center.x - 70.0f, center.y - 140.0f - pos }; if (laneIdx == 1) return { center.x + 70.0f, center.y + 140.0f + pos };
        if (laneIdx == 2) return { center.x + 140.0f + pos, center.y - 70.0f }; return { center.x - 140.0f - pos, center.y + 70.0f };
    }
    PathInfo info = getPathInfo(laneIdx, turn, center);
    if (-pos <= 280.0f) {
        float t = -pos / 280.0f, u = 1.0f - t;
        return { u*u*info.p0.x + 2.0f*u*t*info.p1.x + t*t*info.p2.x, u*u*info.p0.y + 2.0f*u*t*info.p1.y + t*t*info.p2.y };
    }
    return { info.p2.x + info.exitDir.x * (-pos - 280.0f), info.p2.y + info.exitDir.y * (-pos - 280.0f) };
}

Vector2 getLaneDirection(int laneIdx, TurnType turn, float pos, Vector2 center) {
    Vector2 pA = computeVehiclePosition(laneIdx, turn, pos, center), pB = computeVehiclePosition(laneIdx, turn, pos - 2.0f, center);
    float len = hypot(pB.x - pA.x, pB.y - pA.y); return len == 0.0f ? Vector2{0.0f, 1.0f} : Vector2{(pB.x - pA.x)/len, (pB.y - pA.y)/len};
}

int getExitDir(int laneIdx, TurnType turn) {
    if (laneIdx == 0) return turn == STRAIGHT ? 1 : turn == TURN_RIGHT ? 3 : turn == TURN_LEFT ? 2 : 0;
    if (laneIdx == 1) return turn == STRAIGHT ? 0 : turn == TURN_RIGHT ? 2 : turn == TURN_LEFT ? 3 : 1;
    if (laneIdx == 2) return turn == STRAIGHT ? 3 : turn == TURN_RIGHT ? 0 : turn == TURN_LEFT ? 1 : 2;
    return turn == STRAIGHT ? 2 : turn == TURN_RIGHT ? 1 : turn == TURN_LEFT ? 0 : 3;
}
int getOpposingLane(int laneIdx) { return laneIdx == 0 ? 1 : laneIdx == 1 ? 0 : laneIdx == 2 ? 3 : 2; }
TurnType getTurnTypeForNeighbor(int laneIndex, int exitDir) {
    if (laneIndex == 0) return exitDir == 1 ? STRAIGHT : exitDir == 3 ? TURN_RIGHT : exitDir == 2 ? TURN_LEFT : U_TURN;
    if (laneIndex == 1) return exitDir == 0 ? STRAIGHT : exitDir == 2 ? TURN_RIGHT : exitDir == 3 ? TURN_LEFT : U_TURN;
    if (laneIndex == 2) return exitDir == 3 ? STRAIGHT : exitDir == 0 ? TURN_RIGHT : exitDir == 1 ? TURN_LEFT : U_TURN;
    return exitDir == 2 ? STRAIGHT : exitDir == 1 ? TURN_RIGHT : exitDir == 0 ? TURN_LEFT : U_TURN;
}

bool isPathBlockedByCrash(Vector2 startPos, Vector2 endPos, const std::vector<CrashEvent>& crashes, float radius = 55.0f) {
    for (const auto& crash : crashes) {
        float l2 = pow(endPos.x - startPos.x, 2) + pow(endPos.y - startPos.y, 2), dToSeg = 0.0f;
        if (l2 == 0.0f) dToSeg = hypot(crash.pos.x - startPos.x, crash.pos.y - startPos.y);
        else {
            float t = max(0.0f, min(((crash.pos.x - startPos.x) * (endPos.x - startPos.x) + (crash.pos.y - startPos.y) * (endPos.y - startPos.y)) / l2, 1.0f));
            dToSeg = hypot(crash.pos.x - (startPos.x + t * (endPos.x - startPos.x)), crash.pos.y - (startPos.y + t * (endPos.y - startPos.y)));
        }
        if (dToSeg < radius) return true;
    }
    return false;
}

bool isRouteBlocked(int laneIndex, TurnType turn, float startPos, Vector2 center, const std::vector<CrashEvent>& crashes) {
    float endPos = max(-280.0f, startPos - 350.0f);
    for (float pVal = startPos; pVal >= endPos; pVal -= 15.0f) {
        Vector2 p = computeVehiclePosition(laneIndex, turn, pVal, center);
        for (const auto& crash : crashes) if (hypot(p.x - crash.pos.x, p.y - crash.pos.y) < 55.0f) return true;
    }
    return false;
}

bool isOppositeLaneClearForUTurn(Vector2 destPos, const std::vector<CarRect>& allCars, int myId) {
    for (const auto& car : allCars) {
        if (car.id == myId) continue;
        if (hypot((car.rect.x + 20.0f) - destPos.x, (car.rect.y + 20.0f) - destPos.y) < 65.0f) return false;
    }
    return true;
}

float getSpeedMultiplier(int myLaneIndex, int myId, TurnType myTurn, float myPos, const std::vector<CarRect>& allCars, Vector2 center, const std::vector<CrashEvent>& crashes) {
    // 1. Unconditional halt for crash markers
    for (float step = 0.0f; step <= 80.0f; step += 15.0f) {
        Vector2 p = computeVehiclePosition(myLaneIndex, myTurn, myPos - step, center); 
        for (const auto& crash : crashes) if (hypot(p.x - crash.pos.x, p.y - crash.pos.y) < 60.0f) return 0.0f;
    }
    
    // 2. Pledge: All safe drivers swear NOT to overlap
    Vector2 myCurP = computeVehiclePosition(myLaneIndex, myTurn, myPos, center);
    for (const auto& other : allCars) {
        if (other.id == myId) continue;
        
        Vector2 otherP = { other.rect.x + 20.0f, other.rect.y + 20.0f };
        float curDist = hypot(myCurP.x - otherP.x, myCurP.y - otherP.y);
        
        // Failsafe 1: Instant halt if physically too close
        if (curDist < 42.0f) {
            if (other.laneIndex == myLaneIndex) {
                if (myPos > other.pos) return 0.0f; 
            } else {
                if (other.id < myId) return 0.0f; 
            }
        }


        // Failsafe 2: Look ahead to yield
