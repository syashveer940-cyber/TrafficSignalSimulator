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
