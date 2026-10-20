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
    
    // Explicitly implemented to clear crashed vehicles out of lane memory
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
    
    // Explicitly implemented to prevent compiler errors
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
