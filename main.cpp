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
        for (float step = 10.0f; step <= 80.0f; step += 10.0f) {
            Vector2 futureP = computeVehiclePosition(myLaneIndex, myTurn, myPos - step, center);
            if (hypot(futureP.x - otherP.x, futureP.y - otherP.y) < 45.0f) {
                if (other.laneIndex == myLaneIndex) {
                    if (other.pos < myPos) return 0.0f; 
                } else {
                    if (other.id < myId) return 0.0f; 
                }
            }
        }
    }
    return 1.0f; 
}

class TrafficLane {
private: LightState currentState; int boxX, boxY; Vector2 center;
public:
    Queue* vehicleQueue; std::vector<ExitingVehicle> exitingVehicles; std::map<int, TurnType> vehicleTurns; std::vector<UTurningVehicle> activeUTurns;
    TrafficLane(int lightX, int lightY, Vector2 c) { vehicleQueue = createQueue(); currentState = RED_STATE; boxX = lightX; boxY = lightY; center = c; }
    ~TrafficLane() { destroyQueue(vehicleQueue); }
    void addVehicle(int id, bool isEmergency, bool isReckless, int targetDest, TurnType turn, float startPos) { enqueue(vehicleQueue, id, isEmergency ? 1 : 0, isReckless, targetDest, startPos); vehicleTurns[id] = turn; }
    void getAllCarRects(int laneIdx, std::vector<CarRect>& rects) const {
        for (Vehicle* cur = vehicleQueue->front; cur; cur = cur->next) { Vector2 p = computeVehiclePosition(laneIdx, vehicleTurns.count(cur->id) ? vehicleTurns.at(cur->id) : STRAIGHT, cur->pos, center); rects.push_back({ cur->id, laneIdx, cur->isReckless, cur->pos, { p.x - 20.0f, p.y - 20.0f, 40.0f, 40.0f } }); }
        for (const auto& ev : exitingVehicles) { Vector2 p = computeVehiclePosition(laneIdx, ev.turn, ev.pos, center); rects.push_back({ ev.id, laneIdx, ev.isReckless, ev.pos, { p.x - 20.0f, p.y - 20.0f, 40.0f, 40.0f } }); }
        for (const auto& ut : activeUTurns) { float t = ut.progress, u = 1.0f - t; Vector2 p = { u*u*u*ut.p0.x + 3*u*u*t*ut.p1.x + 3*u*t*t*ut.p2.x + t*t*t*ut.p3.x, u*u*u*ut.p0.y + 3*u*u*t*ut.p1.y + 3*u*t*t*ut.p2.y + t*t*t*ut.p3.y }; rects.push_back({ ut.id, laneIdx, ut.isReckless, 9999.0f, { p.x - 20.0f, p.y - 20.0f, 40.0f, 40.0f } }); }
    }
    void removeCrashedVehicles(const std::vector<int>& crashedIDs) {
        for (auto it = exitingVehicles.begin(); it != exitingVehicles.end(); ) { if (find(crashedIDs.begin(), crashedIDs.end(), it->id) != crashedIDs.end()) it = exitingVehicles.erase(it); else ++it; }
        for (auto it = activeUTurns.begin(); it != activeUTurns.end(); ) { if (find(crashedIDs.begin(), crashedIDs.end(), it->id) != crashedIDs.end()) it = activeUTurns.erase(it); else ++it; }
        for (int id : crashedIDs) { removeVehicleById(vehicleQueue, id); vehicleTurns.erase(id); }
    }
    void updateAnimations(int currentIntId, int laneIndex, const std::vector<CarRect>& allCars, Intersection* neighbors[4], const std::vector<CrashEvent>& crashes, Intersection* allGrid[6]);
    void setState(LightState state) { currentState = state; } LightState getState() const { return currentState; }
    int getQueueSize() const { return vehicleQueue->totalCount; } int getEmergencyCount() const { return vehicleQueue->emergencyCount; }
    void drawCars(int laneIndex) {
        for (Vehicle* cur = vehicleQueue->front; cur; cur = cur->next) { Vector2 p = computeVehiclePosition(laneIndex, vehicleTurns.count(cur->id) ? vehicleTurns.at(cur->id) : STRAIGHT, cur->pos, center); DrawRectangle(p.x - 20, p.y - 20, 40, 40, cur->isEmergency ? RED : BLUE); DrawRectangleLines(p.x - 20, p.y - 20, 40, 40, BLACK); }
        for (const auto& ev : exitingVehicles) { Vector2 p = computeVehiclePosition(laneIndex, ev.turn, ev.pos, center); DrawRectangle(p.x - 20, p.y - 20, 40, 40, ev.isEmergency ? RED : BLUE); DrawRectangleLines(p.x - 20, p.y - 20, 40, 40, BLACK); }
        for (const auto& ut : activeUTurns) { float t = ut.progress, u = 1.0f - t; Vector2 p = { u*u*u*ut.p0.x + 3*u*u*t*ut.p1.x + 3*u*t*t*ut.p2.x + t*t*t*ut.p3.x, u*u*u*ut.p0.y + 3*u*u*t*ut.p1.y + 3*u*t*t*ut.p2.y + t*t*t*ut.p3.y }; DrawRectangle(p.x - 20, p.y - 20, 40, 40, ut.isEmergency ? RED : BLUE); DrawRectangleLines(p.x - 20, p.y - 20, 40, 40, BLACK); }
    }
    void drawLights() {
        DrawRectangle(boxX, boxY, 46, 113, {40, 40, 40, 255}); DrawRectangleLines(boxX, boxY, 46, 113, BLACK);
        DrawCircle(boxX + 23, boxY + 24, 13, currentState == RED_STATE ? Color{255, 40, 40, 255} : Color{100, 0, 0, 255}); DrawCircleLines(boxX + 23, boxY + 24, 13, BLACK);
        DrawCircle(boxX + 23, boxY + 56, 13, currentState == YELLOW_STATE ? Color{255, 230, 20, 255} : Color{100, 100, 0, 255}); DrawCircleLines(boxX + 23, boxY + 56, 13, BLACK);
        DrawCircle(boxX + 23, boxY + 88, 13, currentState == GREEN_STATE ? Color{40, 255, 40, 255} : Color{0, 100, 0, 255}); DrawCircleLines(boxX + 23, boxY + 88, 13, BLACK);
    }
};

class Intersection {
public:
    int id; Vector2 center; TrafficLane* lanes[4]; Intersection* neighbors[4]; int activeLaneIndex;
    Intersection(int intId, float cx, float cy) {
        id = intId; center = { cx, cy }; activeLaneIndex = 0; for (int i = 0; i < 4; i++) neighbors[i] = nullptr;
        lanes[0] = new TrafficLane(cx - 175, cy - 220, center); lanes[1] = new TrafficLane(cx + 145, cy + 145, center);
        lanes[2] = new TrafficLane(cx + 145, cy - 220, center); lanes[3] = new TrafficLane(cx - 175, cy + 145, center);
    }
    ~Intersection() { for(int i = 0; i < 4; i++) delete lanes[i]; }
    void optimizeSignal() {
        int clockwiseOrder[4] = {0, 2, 1, 3}; std::vector<int> emergencyLanes;
        for (int i = 0; i < 4; i++) { lanes[i]->setState(RED_STATE); if (lanes[i]->getEmergencyCount() > 0) emergencyLanes.push_back(i); }
        if (!emergencyLanes.empty()) {
            for (int el : emergencyLanes) lanes[el]->setState(GREEN_STATE);
            for (int k = 0; k < 4; k++) if (lanes[clockwiseOrder[k]]->getEmergencyCount() > 0) activeLaneIndex = clockwiseOrder[(k + 1) % 4];
        } else { lanes[activeLaneIndex]->setState(GREEN_STATE); activeLaneIndex = activeLaneIndex == 0 ? 2 : (activeLaneIndex == 2 ? 1 : (activeLaneIndex == 1 ? 3 : 0)); }
    }
    void update(std::vector<CarRect>& allCars, const std::vector<CrashEvent>& crashes, Intersection* grid[6]) { for (int i = 0; i < 4; i++) lanes[i]->updateAnimations(id, i, allCars, neighbors, crashes, grid); }
    void removeCrashedVehicles(const std::vector<int>& crashedIDs) { for (int i = 0; i < 4; i++) lanes[i]->removeCrashedVehicles(crashedIDs); }
    void drawStopLines() { DrawRectangle(center.x - 140, center.y - 144, 140, 4, WHITE); DrawRectangle(center.x, center.y + 140, 140, 4, WHITE); DrawRectangle(center.x + 140, center.y - 140, 4, 140, WHITE); DrawRectangle(center.x - 144, center.y, 4, 140, WHITE); }
};

int getBFSPathDistance(int startId, int targetId, const std::vector<CrashEvent>& crashes, Intersection* grid[6]) {
    if (startId == targetId) return 0;
    std::vector<int> dist(6, -1), q; dist[startId] = 0; q.push_back(startId); int head = 0;
    while (head < (int)q.size()) {
        int curr = q[head++]; if (curr == targetId) return dist[curr];
        for (int dir = 0; dir < 4; dir++) { 
            Intersection* nbr = grid[curr]->neighbors[dir]; 
            if (nbr != nullptr && dist[nbr->id] == -1 && !isPathBlockedByCrash(grid[curr]->center, nbr->center, crashes, 90.0f)) { 
                dist[nbr->id] = dist[curr] + 1; q.push_back(nbr->id); 
            } 
        }
    }
    return 99999; 
}

int getShortestPathNextNeighbor(int startId, int targetId, const std::vector<CrashEvent>& crashes, Intersection* grid[6]) {
    if (startId == targetId) return startId;
    std::vector<int> parent(6, -1), q; std::vector<bool> visited(6, false); visited[startId] = true; q.push_back(startId); int head = 0; bool found = false;
    while (head < (int)q.size()) {
        int curr = q[head++]; if (curr == targetId) { found = true; break; }
        for (int dir = 0; dir < 4; dir++) { 
            Intersection* nbr = grid[curr]->neighbors[dir]; 
            if (nbr != nullptr && !visited[nbr->id] && !isPathBlockedByCrash(grid[curr]->center, nbr->center, crashes, 90.0f)) { 
                visited[nbr->id] = true; parent[nbr->id] = curr; q.push_back(nbr->id); 
            } 
        }
    }
    if (!found) return -1;
    int curr = targetId; while (parent[curr] != -1 && parent[curr] != startId) curr = parent[curr]; return curr;
}

void TrafficLane::updateAnimations(int currentIntId, int laneIndex, const std::vector<CarRect>& allCars, Intersection* neighbors[4], const std::vector<CrashEvent>& crashes, Intersection* allGrid[6]) {
    for (auto it = activeUTurns.begin(); it != activeUTurns.end(); ) {
        float dist = max(10.0f, hypot(it->p0.x - it->p3.x, it->p0.y - it->p3.y) * 1.5f); 
        float stepSize = 3.0f / dist;
        
        float nextT = min(1.0f, it->progress + stepSize);
        float u = 1.0f - nextT;
        Vector2 nextP = { u*u*u*it->p0.x + 3*u*u*nextT*it->p1.x + 3*u*nextT*nextT*it->p2.x + nextT*nextT*nextT*it->p3.x, u*u*u*it->p0.y + 3*u*u*nextT*it->p1.y + 3*u*nextT*nextT*it->p2.y + nextT*nextT*nextT*it->p3.y };
        
        bool blocked = false;
        for (const auto& other : allCars) {
            if (other.id != it->id) {
                float distToOther = hypot(nextP.x - (other.rect.x + 20.0f), nextP.y - (other.rect.y + 20.0f));
                if (distToOther < 46.0f) { 
                    if (other.id < it->id || other.pos < -280.0f || distToOther < 38.0f) blocked = true;
                }
            }
        }
        
        if (!blocked) it->progress += stepSize;
        
        if (it->progress >= 1.0f) {
            TrafficLane* destLaneObj = allGrid[currentIntId]->lanes[it->destLaneIdx];
            if (it->destIsQueue) {
                insertIntoQueueSorted(destLaneObj->vehicleQueue, it->id, it->isEmergency ? 1 : 0, it->isReckless, it->targetDest, it->destPos, it->uTurnCount);
                int nextNbrId = getShortestPathNextNeighbor(currentIntId, it->targetDest, crashes, allGrid); TurnType chosenTurn = STRAIGHT;
                if (nextNbrId != -1 && nextNbrId != currentIntId) { for (int d = 0; d < 4; d++) if (allGrid[currentIntId]->neighbors[d] != nullptr && allGrid[currentIntId]->neighbors[d]->id == nextNbrId) { chosenTurn = getTurnTypeForNeighbor(it->destLaneIdx, d); break; } }
                destLaneObj->vehicleTurns[it->id] = chosenTurn;
            } else destLaneObj->exitingVehicles.push_back({ it->id, it->isEmergency, it->isReckless, it->targetDest, STRAIGHT, it->destPos, it->uTurnCount });
            it = activeUTurns.erase(it);
        } else ++it;
    }

    for (auto it = exitingVehicles.begin(); it != exitingVehicles.end(); ) {
        bool stuck = false;
        if (isRouteBlocked(laneIndex, it->turn, it->pos, center, crashes)) {
            if (it->pos >= -140.0f) { 
                TurnType candidates[3] = { STRAIGHT, TURN_LEFT, TURN_RIGHT };
                TurnType bestTurn = it->turn; int bestDist = 99999;
                for(TurnType t : candidates) {
                    if (!isRouteBlocked(laneIndex, t, it->pos, center, crashes)) {
                        Intersection* nbr = neighbors[getExitDir(laneIndex, t)];
                        if (nbr != nullptr) {
                            int dist = getBFSPathDistance(nbr->id, it->targetDest, crashes, allGrid);
                            if (dist < bestDist) { bestDist = dist; bestTurn = t; }
                        }
                    }
                }
                if (bestDist < 99999) it->turn = bestTurn; else stuck = true;
            } else {
                stuck = true; 
            }
        }
        
        float speedMult = getSpeedMultiplier(laneIndex, it->id, it->turn, it->pos, allCars, center, crashes);
        if (stuck) speedMult = 0.0f; 
        
        it->pos -= 4.0f * speedMult;
        
        if (it->pos <= -600.0f) {
            int exitDir = getExitDir(laneIndex, it->turn); Intersection* neighbor = neighbors[exitDir];
            if (neighbor != nullptr && currentIntId != it->targetDest) {
                int targetLane = (exitDir == 0) ? 1 : (exitDir == 1) ? 0 : (exitDir == 2) ? 3 : 2, nextNbrId = getShortestPathNextNeighbor(neighbor->id, it->targetDest, crashes, allGrid); TurnType chosenTurn = STRAIGHT;
                if (nextNbrId != -1 && nextNbrId != neighbor->id) { for (int d = 0; d < 4; d++) if (neighbor->neighbors[d] != nullptr && neighbor->neighbors[d]->id == nextNbrId) { chosenTurn = getTurnTypeForNeighbor(targetLane, d); break; } }
                else { std::vector<TurnType> validTurns = { STRAIGHT, TURN_RIGHT, TURN_LEFT, U_TURN }; for (TurnType candidate : validTurns) if (!isRouteBlocked(targetLane, candidate, -500.0f, neighbor->center, crashes)) { chosenTurn = candidate; break; } }
                neighbor->lanes[targetLane]->addVehicle(it->id, it->isEmergency, it->isReckless, it->targetDest, chosenTurn, 600.0f); it = exitingVehicles.erase(it);
            // PLEDGE: Never disappear until truly out of the physics world (-5000)
            } else if (it->pos <= -5000.0f) { 
                it = exitingVehicles.erase(it); 
            } else ++it; 
        } else ++it;
    }

    Vehicle *current = vehicleQueue->front, *prevCar = nullptr; int queueIndex = 0;
    while (current != nullptr) {
        TurnType myTurn = vehicleTurns.count(current->id) ? vehicleTurns[current->id] : STRAIGHT;
        bool stuck = false;
        
        if (isRouteBlocked(laneIndex, myTurn, current->pos, center, crashes)) {
            if (current->pos <= 0.0f) { 
                TurnType candidates[3] = { STRAIGHT, TURN_LEFT, TURN_RIGHT };
                TurnType bestTurn = myTurn; int bestDist = 99999;
                for(TurnType t : candidates) {
                    if (!isRouteBlocked(laneIndex, t, current->pos, center, crashes)) {
                        Intersection* nbr = neighbors[getExitDir(laneIndex, t)];
                        if (nbr != nullptr) {
                            int dist = getBFSPathDistance(nbr->id, current->targetDest, crashes, allGrid);
                            if (dist < bestDist) { bestDist = dist; bestTurn = t; }
                        }
                    }
                }
                if (bestDist < 99999) { vehicleTurns[current->id] = bestTurn; myTurn = bestTurn; } 
                else { stuck = true; } 
            } 
            else if (current->pos > 0.0f) {
                int oppLane = getOpposingLane(laneIndex);
                int exitDir = getExitDir(oppLane, STRAIGHT);
                Intersection* uTurnNeighbor = neighbors[exitDir];
                bool uTurnViable = false;
                
                float destPosVal = -current->pos - 280.0f;
                if (uTurnNeighbor != nullptr && !isRouteBlocked(oppLane, STRAIGHT, destPosVal, center, crashes)) {
                    int dist = getBFSPathDistance(uTurnNeighbor->id, current->targetDest, crashes, allGrid);
                    if (dist < 99999) uTurnViable = true;
                }
                
                Vector2 curP = computeVehiclePosition(laneIndex, myTurn, current->pos, center);
                Vector2 destP = computeVehiclePosition(oppLane, STRAIGHT, destPosVal, center);
                if (uTurnViable && isOppositeLaneClearForUTurn(destP, allCars, current->id)) {
                    UTurningVehicle ut = { current->id, current->isEmergency == 1, current->isReckless, current->targetDest, myTurn, current->pos, destPosVal, oppLane, false, curP, {0,0}, {0,0}, {0,0}, 0.0f, current->uTurnCount + 1 };
                    Vector2 d0 = getLaneDirection(laneIndex, ut.originTurn, ut.startPos, center), d3 = getLaneDirection(ut.destLaneIdx, STRAIGHT, ut.destPos, center);
                    ut.p1 = { ut.p0.x + d0.x * 120.0f, ut.p0.y + d0.y * 120.0f }; ut.p3 = computeVehiclePosition(ut.destLaneIdx, STRAIGHT, ut.destPos, center); ut.p2 = { ut.p3.x - d3.x * 120.0f, ut.p3.y - d3.y * 120.0f };
                    activeUTurns.push_back(ut); int cid = current->id; vehicleTurns.erase(cid); removeVehicleById(vehicleQueue, cid); current = vehicleQueue->front; queueIndex = 0; prevCar = nullptr; continue;
                } else {
                    stuck = true; 
                }
            }
        }

        float speedMult = getSpeedMultiplier(laneIndex, current->id, myTurn, current->pos, allCars, center, crashes);
        if (stuck) speedMult = 0.0f; 

        if (queueIndex == 0) {
            if (currentState == GREEN_STATE || current->isReckless || (currentState == YELLOW_STATE && current->pos <= 20.0f)) {
                if (current->isReckless) {
                    if (speedMult > 0.0f) current->pos -= 3.5f * speedMult;
                } else {
                    if (speedMult > 0.0f) { current->pos -= 3.5f * speedMult; }
                }
                
                if (current->pos <= 0.0f) {
                    ExitingVehicle ev = { current->id, current->isEmergency == 1, current->isReckless, current->targetDest, myTurn, current->pos, current->uTurnCount };
                    int nextNbrId = getShortestPathNextNeighbor(currentIntId, ev.targetDest, crashes, allGrid); TurnType bestTurn = myTurn;
                    if (nextNbrId != -1 && nextNbrId != currentIntId) { for (int d = 0; d < 4; d++) if (neighbors[d] != nullptr && neighbors[d]->id == nextNbrId) { bestTurn = getTurnTypeForNeighbor(laneIndex, d); break; } }
                    else { std::vector<TurnType> turnOptions = { myTurn, STRAIGHT, TURN_RIGHT, TURN_LEFT, U_TURN }; for (TurnType candidate : turnOptions) if (!isRouteBlocked(laneIndex, candidate, -500.0f, center, crashes)) { bestTurn = candidate; break; } }
                    ev.turn = bestTurn; exitingVehicles.push_back(ev); vehicleTurns.erase(current->id); delete dequeue(vehicleQueue); current = vehicleQueue->front; continue;
                }
            } else if (current->pos > 0.0f) { 
                if (speedMult > 0.0f) { current->pos -= 3.5f * speedMult; if (current->pos < 0.0f) current->pos = 0.0f; } 
            }
        } else {
            // PLEDGE 1: Safe queueing. Mathematical snap guarantees NO OVERLAP in lines
            float targetPos = max(queueIndex * 60.0f, prevCar != nullptr ? prevCar->pos + 60.0f : 0.0f);
            if (current->pos < targetPos) {
                current->pos = targetPos; 
            } else if (current->pos > targetPos && speedMult > 0.0f) { 
                current->pos -= 3.5f * speedMult; 
                if (current->pos < targetPos) current->pos = targetPos; 
            }
        }
        prevCar = current; current = current->next; queueIndex++;
    }
}

class IntersectionSimulator {
private:
    Intersection* grid[6]; std::vector<Skyscraper> buildings; std::vector<CrashEvent> crashes; int vehicleCounter, frameCounter, lastCrashFrame; Camera2D camera; float speedMultiplier, timeAccumulator; Rectangle speedButtonRect;
    void generateCity() {
        int xRanges[4][2] = { {-1060, -140}, {140, 1060}, {1340, 2260}, {2540, 3460} };
        int yRanges[3][2] = { {-860, 60}, {340, 1260}, {1540, 2460} };
        Color colors[8] = { {70, 100, 130, 255}, {140, 85, 75, 255}, {110, 115, 85, 255}, {120, 110, 105, 255}, {85, 95, 110, 255}, {105, 85, 110, 255}, {70, 115, 110, 255}, {140, 125, 105, 255} };
        for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) for (int x = xRanges[c][0] + 35; x < xRanges[c][1] - 85; x += 50) for (int y = yRanges[r][0] + 35; y < yRanges[r][1] - 85; y += 50) {
            if (rand() % 100 < 50) {
                Skyscraper b; int w = 28 + rand() % 12, h = 28 + rand() % 12; b.base = { (float)x + (50 - w) / 2.0f, (float)y + (50 - h) / 2.0f, (float)w, (float)h };
                b.height = (rand() % 100) / 100.0f * 0.15f + 0.08f; b.col = colors[rand() % 8];
                Rectangle checkRect = { b.base.x - 40, b.base.y - 40, b.base.width + 100, b.base.height + 100 }; bool overlap = false;
                for (int cx : {0, 1200, 2400}) for (int cy : {200, 1400}) { Rectangle lights[4] = { {(float)cx-175, (float)cy-220, 46, 113}, {(float)cx+145, (float)cy+145, 46, 113}, {(float)cx+145, (float)cy-220, 46, 113}, {(float)cx-175, (float)cy+145, 46, 113} }; for(int l=0; l<4; l++) if (CheckCollisionRecs(checkRect, lights[l])) overlap = true; }
                if (!overlap) buildings.push_back(b);
            }
        }
        std::sort(buildings.begin(), buildings.end(), [](const Skyscraper& a, const Skyscraper& b) { return pow(a.base.x + a.base.width/2 - 1200, 2) + pow(a.base.y + a.base.height/2 - 800, 2) > pow(b.base.x + b.base.width/2 - 1200, 2) + pow(b.base.y + b.base.height/2 - 800, 2); });
    }
    void drawAllRoads() {
        for (int x : {0, 1200, 2400}) { DrawRectangle(x - 140, -1200, 280, 4200, GRAY); for (int y = -1200; y < 3000; y += 40) { bool inInt = false; for (int cy : {200, 1400}) if (abs(y - cy) < 140) inInt = true; if (!inInt) DrawRectangle(x - 2, y + 10, 4, 20, YELLOW); } }
        for (int y : {200, 1400}) { DrawRectangle(-1600, y - 140, 5600, 280, GRAY); for (int x = -1600; x < 4000; x += 40) { bool inInt = false; for (int cx : {0, 1200, 2400}) if (abs(x - cx) < 140) inInt = true; if (!inInt) DrawRectangle(x + 10, y - 2, 20, 4, YELLOW); } }
    }
public:
    IntersectionSimulator() {
        grid[0] = new Intersection(0, 0, 200); grid[1] = new Intersection(1, 1200, 200); grid[2] = new Intersection(2, 2400, 200); grid[3] = new Intersection(3, 0, 1400); grid[4] = new Intersection(4, 1200, 1400); grid[5] = new Intersection(5, 2400, 1400);
        for (int r = 0; r < 2; r++) for (int c = 0; c < 3; c++) { int i = r * 3 + c; if (r > 0) grid[i]->neighbors[0] = grid[(r - 1) * 3 + c]; if (r < 1) grid[i]->neighbors[1] = grid[(r + 1) * 3 + c]; if (c < 2) grid[i]->neighbors[2] = grid[r * 3 + c + 1]; if (c > 0) grid[i]->neighbors[3] = grid[r * 3 + c - 1]; }
        vehicleCounter = 1000; frameCounter = 0; lastCrashFrame = -5400; srand(time(0)); camera = { 0 }; camera.target = { 1200.0f, 800.0f }; camera.offset = { 600.0f, 400.0f }; camera.rotation = 0.0f; camera.zoom = 0.25f;
        generateCity(); speedMultiplier = 1.0f; timeAccumulator = 0.0f; speedButtonRect = { 20.0f, 730.0f, 100.0f, 50.0f };
    }
    ~IntersectionSimulator() { for(int i = 0; i < 6; i++) delete grid[i]; }
    void generateTraffic() { 
        int active = 0; 
        for(int i=0; i<6; i++) for(int j=0; j<4; j++) active += grid[i]->lanes[j]->getQueueSize() + grid[i]->lanes[j]->exitingVehicles.size() + grid[i]->lanes[j]->activeUTurns.size();
        if (active > 160) return; 

        for (int i = 0; i < 6; i++) for (int lane = 0; lane < 4; lane++) {
            if (grid[i]->neighbors[lane] == nullptr && rand() % 1000 < 20) {
                bool isReck = false;
                // Force a reckless driver every 5400 frames (30s at 3x) to guarantee your crash timing
                if (crashes.empty() && frameCounter - lastCrashFrame > 5400) isReck = true;
                grid[i]->lanes[lane]->addVehicle(++vehicleCounter, (rand() % 100) < 5, isReck, rand() % 6, getTurnTypeForNeighbor(lane, rand() % 4), 1500.0f); 
            }
        }
    }
    void updateLogic() {
        frameCounter++;
        if (frameCounter % 180 == 135) for (int i = 0; i < 6; i++) for (int j = 0; j < 4; j++) if (grid[i]->lanes[j]->getState() == GREEN_STATE) grid[i]->lanes[j]->setState(YELLOW_STATE);
        if (frameCounter % 180 == 0) for (int i = 0; i < 6; i++) grid[i]->optimizeSignal();
        if (frameCounter % 12 == 0) generateTraffic();
        std::vector<CarRect> allGlobalCars; for (int i = 0; i < 6; i++) for (int j = 0; j < 4; j++) grid[i]->lanes[j]->getAllCarRects(j, allGlobalCars);
        
        for (size_t i = 0; i < allGlobalCars.size(); i++) for (size_t j = i + 1; j < allGlobalCars.size(); j++) {
            Vector2 p1 = { allGlobalCars[i].rect.x + 20.0f, allGlobalCars[i].rect.y + 20.0f };
            Vector2 p2 = { allGlobalCars[j].rect.x + 20.0f, allGlobalCars[j].rect.y + 20.0f };
            
            if (hypot(p1.x - p2.x, p1.y - p2.y) < 38.0f && allGlobalCars[i].id != allGlobalCars[j].id) {
                if (allGlobalCars[i].isReckless || allGlobalCars[j].isReckless) {
                    if (crashes.empty() && frameCounter - lastCrashFrame > 5400) { 
                        Vector2 crashPos = { (p1.x + p2.x)/2.0f, (p1.y + p2.y)/2.0f };
                        // Crash stays active for exactly 1800 frames (30 seconds at 1x speed).
                        crashes.push_back({crashPos, 1800, allGlobalCars[i].id, allGlobalCars[j].id}); 
                        lastCrashFrame = frameCounter;
                    }
                }
            }
        }
        
        // When the crash vanishes, the EXACT TWO CARS are PERMANENTLY ERASED so they never reappear
        std::vector<int> crashedCarIDsToErase;
        for (auto it = crashes.begin(); it != crashes.end(); ) { 
            it->framesLeft--; 
            if (it->framesLeft <= 0) { 
                crashedCarIDsToErase.push_back(it->car1Id);
                crashedCarIDsToErase.push_back(it->car2Id);
                it = crashes.erase(it); 
            } else {
                ++it; 
            }
        }
        if (!crashedCarIDsToErase.empty()) { 
            std::sort(crashedCarIDsToErase.begin(), crashedCarIDsToErase.end()); 
            crashedCarIDsToErase.erase(std::unique(crashedCarIDsToErase.begin(), crashedCarIDsToErase.end()), crashedCarIDsToErase.end()); 
            for (int i = 0; i < 6; i++) grid[i]->removeCrashedVehicles(crashedCarIDsToErase); 
        }

        for (int i = 0; i < 6; i++) { std::vector<CarRect> localCars; for (int j = 0; j < 4; j++) grid[i]->lanes[j]->getAllCarRects(j, localCars); grid[i]->update(localCars, crashes, grid); }
    }
    void drawLogic() {
        BeginDrawing(); ClearBackground({35, 40, 45, 255}); BeginMode2D(camera); drawAllRoads();
        for (int i = 0; i < 6; i++) grid[i]->drawStopLines();
        for (const auto& b : buildings) DrawRectangleRec({b.base.x + b.height * 60.0f, b.base.y + b.height * 40.0f, b.base.width, b.base.height}, {15, 20, 25, 120});
        for (int i = 0; i < 6; i++) for (int j = 0; j < 4; j++) grid[i]->lanes[j]->drawCars(j);
        Vector2 focal = { 1200.0f, 800.0f };
        for (const auto& b : buildings) {
            int layers = (int)(b.height * 50);
            for (int i = 0; i < layers; i++) DrawRectangleRec({b.base.x + (b.base.x + b.base.width/2 - focal.x) * b.height * 0.3f * ((float)i/layers), b.base.y + (b.base.y + b.base.height/2 - focal.y) * b.height * 0.3f * ((float)i/layers), b.base.width, b.base.height}, AdjustBrightness(b.col, ((float)i/layers) * 0.5f - 0.25f));
            float ox = (b.base.x + b.base.width/2 - focal.x) * b.height * 0.3f, oy = (b.base.y + b.base.height/2 - focal.y) * b.height * 0.3f;
            DrawRectangleLinesEx({b.base.x + ox, b.base.y + oy, b.base.width, b.base.height}, 2.0f, AdjustBrightness(b.col, 0.2f)); DrawRectangleRec({b.base.x + ox + b.base.width*0.35f, b.base.y + oy + b.base.height*0.35f, b.base.width*0.3f, b.base.height*0.3f}, AdjustBrightness(b.col, 0.4f));
        }
        for (int i = 0; i < 6; i++) for (int j = 0; j < 4; j++) grid[i]->lanes[j]->drawLights();
        for (const auto& crash : crashes) { DrawCircle(crash.pos.x, crash.pos.y, 42, MAROON); DrawCircle(crash.pos.x, crash.pos.y, 36, RED); DrawCircleLines(crash.pos.x, crash.pos.y, 36, WHITE); DrawLineEx({crash.pos.x - 18, crash.pos.y - 18}, {crash.pos.x + 18, crash.pos.y + 18}, 8, WHITE); DrawLineEx({crash.pos.x - 18, crash.pos.y + 18}, {crash.pos.x + 18, crash.pos.y - 18}, 8, WHITE); }
        EndMode2D(); DrawRectangle(0, 0, 440, 60, Fade(BLACK, 0.7f)); DrawText("Dynamic Traffic Signal Optimization Simulator", 10, 10, 20, WHITE); DrawText("Blue = Normal | Red = Emergency", 10, 35, 15, LIGHTGRAY);
        DrawRectangleRec(speedButtonRect, CheckCollisionPointRec(GetMousePosition(), speedButtonRect) ? Color{255, 255, 255, 90} : Color{255, 255, 255, 50}); DrawRectangleLinesEx(speedButtonRect, 2.0f, WHITE);
        const char* speedDisplay = TextFormat((speedMultiplier == (int)speedMultiplier) ? "Speed: %.0fx" : "Speed: %.1fx", speedMultiplier);
        DrawText(speedDisplay, speedButtonRect.x + (speedButtonRect.width - MeasureText(speedDisplay, 20)) / 2, speedButtonRect.y + 15, 20, WHITE); EndDrawing();
    }
    void runFrame() {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), speedButtonRect)) if ((speedMultiplier += 0.5f) > 3.0f) speedMultiplier = 1.0f;
        timeAccumulator += speedMultiplier; while (timeAccumulator >= 1.0f) { updateLogic(); timeAccumulator -= 1.0f; } drawLogic();
    }
};

int main() {
    InitWindow(1200, 800, "Dynamic Traffic Signal Optimization Simulator"); SetTargetFPS(60); IntersectionSimulator sim;
    while (!WindowShouldClose()) sim.runFrame();
    CloseWindow(); return 0;
}