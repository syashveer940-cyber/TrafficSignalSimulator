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
DrawCircleV({p.x+dir.x*20-pR.x*11,p.y+dir.y*20-pR.y*11},3.2f,Color{255,255,220,255});
            if (tod==TIME_NIGHT) DrawCircleV({p.x+dir.x*35,p.y+dir.y*35},15.0f,Fade(YELLOW,0.18f));
            DrawCircleV({p.x-dir.x*20+pR.x*11,p.y-dir.y*20+pR.y*11},2.8f,RED); DrawCircleV({p.x-dir.x*20-pR.x*11,p.y-dir.y*20-pR.y*11},2.8f,RED);
        }
    }
public:
    Queue* vQ; std::vector<ExitingVehicle> exVs; std::map<int,TurnType> vTurns; std::vector<UTurningVehicle> aUTs;
    TrafficLane(int x, int y, Vector2 c) { vQ = createQueue(); state = RED_STATE; bX = x; bY = y; center = c; }
    ~TrafficLane() { destroyQueue(vQ); }
    void addVehicle(int id, bool emg, bool reck, int dest, TurnType t, float sP, float mS, int oL) { enqueue(vQ,id,emg?1:0,reck,dest,sP,mS,oL); vTurns[id]=t; }
    void getAllCarRects(int l, std::vector<CarRect>& rs) const {
        for (Vehicle* c=vQ->front; c; c=c->next) { if(c->pos>1400.f)continue; Vector2 p=computeVehiclePosition(l,vTurns.count(c->id)?vTurns.at(c->id):STRAIGHT,c->pos,center); rs.push_back({c->id,l,c->isReckless,c->pos,{p.x-20,p.y-20,40,40}}); }
        for (const auto& ev:exVs) { if(ev.pos<-1400.f)continue; Vector2 p=computeVehiclePosition(l,ev.turn,ev.pos,center); rs.push_back({ev.id,l,ev.isReckless,ev.pos,{p.x-20,p.y-20,40,40}}); }
        for (const auto& ut:aUTs) { float t=ut.progress,u=1.f-t; Vector2 p={u*u*u*ut.p0.x+3*u*u*t*ut.p1.x+3*u*t*t*ut.p2.x+t*t*t*ut.p3.x, u*u*u*ut.p0.y+3*u*u*t*ut.p1.y+3*u*t*t*ut.p2.y+t*t*t*ut.p3.y}; rs.push_back({ut.id,l,ut.isReckless,9999.f,{p.x-20,p.y-20,40,40}}); }
    }
    void removeCrashedVehicles(const std::vector<int>& ids) {
        for(auto it=exVs.begin();it!=exVs.end();) { if(find(ids.begin(),ids.end(),it->id)!=ids.end()) it=exVs.erase(it); else ++it; }
        for(auto it=aUTs.begin();it!=aUTs.end();) { if(find(ids.begin(),ids.end(),it->id)!=ids.end()) it=aUTs.erase(it); else ++it; }
        for(int id:ids) { removeVehicleById(vQ,id); vTurns.erase(id); }
    }
    void updateAnims(int cId, int l, const std::vector<CarRect>& cs, Intersection* nb[4], Intersection* g[6], float wF, float dH, const std::vector<CrashEvent>& crs);
void setState(LightState s) { state = s; } LightState getState() const { return state; } int getQueueSize() const { return vQ->totalCount; } int getEmergencyCount() const { return vQ->emergencyCount; }
    void drawCars(int l, TimeOfDay tod, const std::vector<CrashEvent>& crs) {
        auto isCrash = [&](int id){for(const auto& c:crs) if(c.car1Id==id||c.car2Id==id) return true; return false;};
        for(Vehicle* c=vQ->front; c; c=c->next) { if(c->pos>1400.f || isCrash(c->id)) continue; TurnType t=vTurns.count(c->id)?vTurns.at(c->id):STRAIGHT; drawVehicle(computeVehiclePosition(l,t,c->pos,center),getLaneDirection(l,t,c->pos,center),c->isEmergency==1,c->isReckless,tod); }
        for(const auto& e:exVs) { if(isCrash(e.id)) continue; drawVehicle(computeVehiclePosition(l,e.turn,e.pos,center),getLaneDirection(l,e.turn,e.pos,center),e.isEmergency,e.isReckless,tod); }
        for(const auto& u:aUTs) { if(isCrash(u.id)) continue; float t=u.progress,v=1.f-t; Vector2 dir={u.p3.x-u.p0.x,u.p3.y-u.p0.y}; float dL=hypot(dir.x,dir.y); if(dL>0.001f){dir.x/=dL; dir.y/=dL;} else dir={0,1}; drawVehicle({v*v*v*u.p0.x+3*v*v*t*u.p1.x+3*v*t*t*u.p2.x+t*t*t*u.p3.x,v*v*v*u.p0.y+3*v*v*t*u.p1.y+3*v*t*t*u.p2.y+t*t*t*u.p3.y},dir,u.isEmergency,u.isReckless,tod); }
    }
    void drawLights() {
        DrawRectangle(bX,bY,46,113,{40,40,40,255}); DrawRectangleLines(bX,bY,46,113,BLACK);
        DrawCircle(bX+23,bY+24,13,state==RED_STATE?Color{255,40,40,255}:Color{100,0,0,255}); DrawCircleLines(bX+23,bY+24,13,BLACK);
        DrawCircle(bX+23,bY+56,13,state==YELLOW_STATE?Color{255,230,20,255}:Color{100,100,0,255}); DrawCircleLines(bX+23,bY+56,13,BLACK);
DrawCircle(bX+23,bY+88,13,state==GREEN_STATE?Color{40,255,40,255}:Color{0,100,0,255}); DrawCircleLines(bX+23,bY+88,13,BLACK);
    }
};

class Intersection {
public:
    int id; Vector2 center; TrafficLane* lanes[4]; Intersection* neighbors[4]; int actL;
    Intersection(int iId, float cx, float cy) { id=iId; center={cx,cy}; actL=0; for(int i=0;i<4;i++) neighbors[i]=nullptr; lanes[0]=new TrafficLane(cx-175,cy-220,center); lanes[1]=new TrafficLane(cx+145,cy+145,center); lanes[2]=new TrafficLane(cx+145,cy-220,center); lanes[3]=new TrafficLane(cx-175,cy+145,center); }
    ~Intersection() { for(int i=0;i<4;i++) delete lanes[i]; }
    void optimizeSignal() { int order[4]={0,2,1,3}; std::vector<int> emg; for(int i=0;i<4;i++) { lanes[i]->setState(RED_STATE); if(lanes[i]->getEmergencyCount()>0) emg.push_back(i); } if(!emg.empty()) { for(int e:emg) lanes[e]->setState(GREEN_STATE); for(int k=0;k<4;k++) if(lanes[order[k]]->getEmergencyCount()>0) actL=order[(k+1)%4]; } else { lanes[actL]->setState(GREEN_STATE); actL=actL==0?2:(actL==2?1:(actL==1?3:0)); } }
    void update(std::vector<CarRect>& cs, Intersection* g[6], float wF, float dH, const std::vector<CrashEvent>& crs) { for(int i=0;i<4;i++) lanes[i]->updateAnims(id,i,cs,neighbors,g,wF,dH,crs); }
void removeCrashedVehicles(const std::vector<int>& ids) { for(int i=0;i<4;i++) lanes[i]->removeCrashedVehicles(ids); }
    void drawStopLines() { DrawRectangle(center.x-140,center.y-144,140,4,WHITE); DrawRectangle(center.x,center.y+140,140,4,WHITE); DrawRectangle(center.x+140,center.y-140,4,140,WHITE); DrawRectangle(center.x-144,center.y,4,140,WHITE); }
};

int getShortestPath(int sId, int tId, Intersection* g[6]) {
    if(sId==tId) return sId; std::vector<int> p(6,-1), q; std::vector<bool> v(6,false); v[sId]=true; q.push_back(sId); int h=0; bool f=false;
    while(h<(int)q.size()){ int c=q[h++]; if(c==tId){ f=true; break; } for(int d=0;d<4;d++) { Intersection* n=g[c]->neighbors[d]; if(n!=nullptr&&!v[n->id]){ v[n->id]=true; p[n->id]=c; q.push_back(n->id); } } }
    if(!f) return -1; int c=tId; while(p[c]!=-1&&p[c]!=sId) c=p[c]; return c;
}

void TrafficLane::updateAnims(int cId, int lI, const std::vector<CarRect>& cs, Intersection* nb[4], Intersection* g[6], float wF, float dH, const std::vector<CrashEvent>& crs) {
    for (auto it = aUTs.begin(); it != aUTs.end(); ) {
        bool isCr = false; for(const auto& c:crs) if(c.car1Id==it->id || c.car2Id==it->id) isCr=true; if(isCr) { ++it; continue; }
        float d = max(10.f, hypot(it->p0.x-it->p3.x, it->p0.y-it->p3.y)*1.5f), step = (it->isReckless?4.5f:3.f)*wF/d, nT = min(1.f, it->progress+step), u = 1.f-nT;
        Vector2 nP = {u*u*u*it->p0.x+3*u*u*nT*it->p1.x+3*u*nT*nT*it->p2.x+nT*nT*nT*it->p3.x, u*u*u*it->p0.y+3*u*u*nT*it->p1.y+3*u*nT*nT*it->p2.y+nT*nT*nT*it->p3.y};
        bool blk = false; if (!it->isReckless) { for(const auto& o:cs) if(o.id!=it->id && hypot(nP.x-(o.rect.x+20), nP.y-(o.rect.y+20))<48.f) { blk=true; break; } }
        if (!blk) it->progress += step;
        if (it->progress >= 1.f) {
            TrafficLane* dL = g[cId]->lanes[it->destLaneIdx];
            if (it->destIsQueue) {
                insertIntoQueueSorted(dL->vQ, it->id, it->isEmergency?1:0, it->isReckless, it->targetDest, it->destPos, it->uTurnCount, it->originLane);
void removeCrashedVehicles(const std::vector<int>& ids) { for(int i=0;i<4;i++) lanes[i]->removeCrashedVehicles(ids); }
    void drawStopLines() { DrawRectangle(center.x-140,center.y-144,140,4,WHITE); DrawRectangle(center.x,center.y+140,140,4,WHITE); DrawRectangle(center.x+140,center.y-140,4,140,WHITE); DrawRectangle(center.x-144,center.y,4,140,WHITE); }
};

int getShortestPath(int sId, int tId, Intersection* g[6]) {
    if(sId==tId) return sId; std::vector<int> p(6,-1), q; std::vector<bool> v(6,false); v[sId]=true; q.push_back(sId); int h=0; bool f=false;
    while(h<(int)q.size()){ int c=q[h++]; if(c==tId){ f=true; break; } for(int d=0;d<4;d++) { Intersection* n=g[c]->neighbors[d]; if(n!=nullptr&&!v[n->id]){ v[n->id]=true; p[n->id]=c; q.push_back(n->id); } } }
    if(!f) return -1; int c=tId; while(p[c]!=-1&&p[c]!=sId) c=p[c]; return c;
}

void TrafficLane::updateAnims(int cId, int lI, const std::vector<CarRect>& cs, Intersection* nb[4], Intersection* g[6], float wF, float dH, const std::vector<CrashEvent>& crs) {
    for (auto it = aUTs.begin(); it != aUTs.end(); ) {
        bool isCr = false; for(const auto& c:crs) if(c.car1Id==it->id || c.car2Id==it->id) isCr=true; if(isCr) { ++it; continue; }
        float d = max(10.f, hypot(it->p0.x-it->p3.x, it->p0.y-it->p3.y)*1.5f), step = (it->isReckless?4.5f:3.f)*wF/d, nT = min(1.f, it->progress+step), u = 1.f-nT;
        Vector2 nP = {u*u*u*it->p0.x+3*u*u*nT*it->p1.x+3*u*nT*nT*it->p2.x+nT*nT*nT*it->p3.x, u*u*u*it->p0.y+3*u*u*nT*it->p1.y+3*u*nT*nT*it->p2.y+nT*nT*nT*it->p3.y};
        bool blk = false; if (!it->isReckless) { for(const auto& o:cs) if(o.id!=it->id && hypot(nP.x-(o.rect.x+20), nP.y-(o.rect.y+20))<48.f) { blk=true; break; } }
        if (!blk) it->progress += step;
        if (it->progress >= 1.f) {
            TrafficLane* dL = g[cId]->lanes[it->destLaneIdx];
            if (it->destIsQueue) {
                insertIntoQueueSorted(dL->vQ, it->id, it->isEmergency?1:0, it->isReckless, it->targetDest, it->destPos, it->uTurnCount, it->originLane);
Vehicle *c = vQ->front, *pC = nullptr; int qI = 0;
    while (c != nullptr) {
        bool isCr = false; for(const auto& cr:crs) if(cr.car1Id==c->id || cr.car2Id==c->id) isCr=true; if(isCr) { pC=c; c=c->next; qI++; continue; }
        TurnType mT = vTurns.count(c->id) ? vTurns[c->id] : STRAIGHT; float sM = 1.0f;
        if (c->isReckless) sM = 1.5f; else if (qI==0 || c->pos<=60.f) sM = getSafeYieldMultiplier(lI, c->id, mT, c->pos, cs, center);
        if (qI==0 || c->isReckless) {
            if (c->isReckless || state==GREEN_STATE || (state==YELLOW_STATE && c->pos<=20.f)) {
                bool blk = false; if (!c->isReckless) { for(const auto& ev:exVs) if(ev.pos>-56.f) { blk=true; break; } }
                if (sM>0.f && !blk) c->pos -= (3.5f*wF)*sM;
                if (c->pos<=0.f && !blk) {
                    ExitingVehicle ev = {c->id, c->isEmergency==1, c->isReckless, c->targetDest, mT, c->pos, c->uTurnCount, c->originLane};
                    int nN = getShortestPath(cId, ev.targetDest, g); TurnType bT = mT;
                    if (nN!=-1&&nN!=cId) { for(int d=0; d<4; d++) if(nb[d]!=nullptr && nb[d]->id==nN) { bT=getTurnTypeForNeighbor(lI,d); break; } }
                    else { bool fE=false; for(int d=0; d<4; d++) if(nb[d]==nullptr) { bT=getTurnTypeForNeighbor(lI,d); fE=true; break; } if(!fE) { std::vector<TurnType> tO={mT,STRAIGHT,TURN_RIGHT,TURN_LEFT}; bT=tO[rand()%tO.size()]; } }
                    ev.turn = bT; exVs.push_back(ev); vTurns.erase(c->id); delete dequeue(vQ); c = vQ->front; continue;
                }
            } else if (c->pos>0.f && sM>0.f) { c->pos -= (3.5f*wF)*sM; if(c->pos<0.f) c->pos=0.f; }
        } else {
            float tP = max(qI*dH, pC!=nullptr ? pC->pos+dH : 0.f);
            if (c->pos<tP) c->pos=tP; else if (c->pos>tP && sM>0.f) { c->pos -= (3.5f*wF)*sM; if(c->pos<tP) c->pos=tP; }
        }
pC=c; c=c->next; qI++;
    }
}

string getCrashLoc(Vector2 p, Intersection* g[6]) {
    int nId=0; float mD=1e9f; for(int i=0; i<6; i++) { float d=hypot(p.x-g[i]->center.x,p.y-g[i]->center.y); if(d<mD){ mD=d; nId=i; } }
    const char* nms[6]={"Int 0 (NW)","Int 1 (N-Mid)","Int 2 (NE)","Int 3 (SW)","Int 4 (S-Mid)","Int 5 (SE)"}; float dx=p.x-g[nId]->center.x, dy=p.y-g[nId]->center.y;
    if(fabs(dx)<=140.f && fabs(dy)<=140.f) return string(nms[nId])+" Center"; return string(nms[nId])+(fabs(dx)>fabs(dy)?(dx>0?" - East Arm":" - West Arm"):(dy>0?" - South Arm":" - North Arm"));
}

class IntersectionSimulator {
private:
    Intersection* grid[6]; std::vector<Skyscraper> bldgs; std::vector<CrashEvent> crashes; int vCounter, fCounter, lastCr, selLane; Camera2D cam; float spdMult, tAcc, sbProg; bool reckAct, sbOpen, lInpAct; TimeOfDay cTime; WeatherType cWeath; const float SB_W=250.f; std::vector<WeatherParticle> parts; std::vector<UpcomingEntryLane> uLanes; std::string lStrs[10];
    void initUpcomingLanes() {
        uLanes = { {0,0,"L1","North-W (Int 0)",Color{0,240,255,255},30,5,0.f,false}, {1,0,"L2","North-Mid (Int 1)",Color{255,235,0,255},45,8,0.f,false}, {2,0,"L3","North-E (Int 2)",Color{255,30,150,255},25,4,0.f,false}, {0,3,"L4","West-Up (Int 0)",Color{255,120,0,255},35,6,0.f,false}, {3,3,"L5","West-Low (Int 3)",Color{30,255,40,255},28,5,0.f,false}, {2,2,"L6","East-Up (Int 2)",Color{190,50,255,255},32,6,0.f,false}, {5,2,"L7","East-Low (Int 5)",Color{255,40,40,255},40,7,0.f,false}, {3,1,"L8","South-W (Int 3)",Color{30,160,255,255},22,4,0.f,false}, {4,1,"L9","South-Mid (Int 4)",Color{255,180,0,255},50,9,0.f,false}, {5,1,"L10","South-E (Int 5)",Color{0,255,190,255},26,5,0.f,false} };
        selLane=0; lInpAct=false; for(int i=0;i<10;i++) lStrs[i]=to_string(uLanes[i].targetPer30s);
    }
    void initParts() { parts.clear(); for(int i=0;i<140;i++) parts.push_back({(float)(rand()%1200),(float)(rand()%800),-1.f+(rand()%100)/100.f*0.5f,6.f+(rand()%100)/100.f*4.f,2.f+(rand()%3)}); }
    void updateParts() {
        for(auto& p:parts){ if(cWeath==WEATHER_RAINY){p.y+=p.speedY*1.5f;p.x+=-2.5f;} else if(cWeath==WEATHER_WINTER){p.y+=1.8f;p.x+=sin(fCounter*0.03f+p.y*0.05f)*1.2f;} else if(cWeath==WEATHER_AUTUMN){p.y+=2.6f;p.x+=1.8f+cos(fCounter*0.02f)*0.8f;} else if(cWeath==WEATHER_SPRING){p.y+=1.4f;p.x+=1.5f;} if(p.y>800){p.y=-10;p.x=rand()%1200;} if(p.x<0)p.x=1200; if(p.x>1200)p.x=0; }
    }
float getWSpeed() const { return cWeath==WEATHER_SUMMER?1.05f:cWeath==WEATHER_SPRING?1.0f:cWeath==WEATHER_AUTUMN?0.92f:cWeath==WEATHER_RAINY?0.82f:0.72f; }
    float getDHead() const { return cWeath==WEATHER_SUMMER?60.f:cWeath==WEATHER_SPRING?62.f:cWeath==WEATHER_AUTUMN?66.f:cWeath==WEATHER_RAINY?72.f:78.f; }
    int getCap() const { int b=cTime==TIME_MORNING?110:cTime==TIME_AFTERNOON?60:cTime==TIME_EVENING?90:20; float m=cWeath==WEATHER_SUMMER?1.06f:cWeath==WEATHER_SPRING?1.f:cWeath==WEATHER_AUTUMN?0.95f:cWeath==WEATHER_RAINY?0.9f:0.8f; return (int)(b*m); }
    void updateTargets() {
        int tt=getCap()/2, bW[10]={5,8,4,6,5,6,7,4,9,5}, tUw=0, lU=-1; for(int i=0;i<10;i++) if(!uLanes[i].isUserCustomized){ tUw+=bW[i]; lU=i; }
        if(tUw>0){ int cS=0; for(int i=0;i<10;i++) if(uLanes[i].isUserCustomized) cS+=uLanes[i].targetPer30s; int rT=max(0,tt-cS), rS=0; for(int i=0;i<10;i++) if(!uLanes[i].isUserCustomized){ int a=i==lU?max(0,rT-rS):round(((float)bW[i]/tUw)*rT); uLanes[i].targetPer30s=max(0,a); rS+=a; if(!lInpAct||selLane!=i) lStrs[i]=to_string(uLanes[i].targetPer30s); } }
    }
    void genCity() {
        int xR[4][2]={{-1460,-140},{140,1060},{1340,2260},{2540,3860}}, yR[3][2]={{-1160,60},{340,1260},{1540,2760}}; Color cls[8]={{70,100,130,255},{140,85,75,255},{110,115,85,255},{120,110,105,255},{85,95,110,255},{105,85,110,255},{70,115,110,255},{140,125,105,255}};
        for(int r=0; r<3; r++) for(int c=0; c<4; c++) {
            float bW=xR[c][1]-xR[c][0], bH=yR[r][1]-yR[r][0], sX=xR[c][0]+(bW-((int)(bW/115.f-1)*115.f+90.f))/2.f, sY=yR[r][0]+(bH-((int)(bH/115.f-1)*115.f+90.f))/2.f;
            for(int i=0; i<(int)(bW/115.f); i++) for(int j=0; j<(int)(bH/115.f); j++) {
                Skyscraper b; b.base={sX+i*115.f,sY+j*115.f,90.f,90.f}; b.col=cls[rand()%8]; bool olap=false;
                for(int cx:{0,1200,2400}) for(int cy:{200,1400}) { Rectangle ls[4]={{(float)cx-175,(float)cy-220,46,113},{(float)cx+145,(float)cy+145,46,113},{(float)cx+145,(float)cy-220,46,113},{(float)cx-175,(float)cy+145,46,113}}; for(int l=0;l<4;l++) if(CheckCollisionRecs({b.base.x-10,b.base.y-10,110,110},ls[l])) olap=true; }
                if(!olap) bldgs.push_back(b);
            }
        }
    }
    void drawRds() {
        for(int x:{0,1200,2400}) { DrawRectangle(x-140,-1200,280,4200,GRAY); for(int y=-1200;y<3000;y+=40){ bool inI=false; for(int cy:{200,1400}) if(abs(y-cy)<140) inI=true; if(!inI) DrawRectangle(x-2,y+10,4,20,YELLOW); } }
        for(int y:{200,1400}) { DrawRectangle(-1600,y-140,5600,280,GRAY); for(int x=-1600;x<4000;x+=40){ bool inI=false; for(int cx:{0,1200,2400}) if(abs(x-cx)<140) inI=true; if(!inI) DrawRectangle(x+10,y-2,20,4,YELLOW); }
}
    void drawZebra() {
        for(size_t i=0; i<uLanes.size(); i++){
            const auto& l=uLanes[i]; Vector2 c=grid[l.intId]->center; bool isS=(i==(size_t)selLane);
            if(l.laneIdx==0){ for(int s=0;s<5;s++) DrawRectangle(c.x-118+s*20,c.y-660,15,48,l.color); if(isS) DrawRectangleLinesEx({c.x-120,c.y-662,98,52},2.f,WHITE); }
            else if(l.laneIdx==1){ for(int s=0;s<5;s++) DrawRectangle(c.x+22+s*20,c.y+660,15,48,l.color); if(isS) DrawRectangleLinesEx({c.x+20,c.y+658,98,52},2.f,WHITE); }
            else if(l.laneIdx==2){ for(int s=0;s<5;s++) DrawRectangle(c.x+740,c.y-118+s*20,48,15,l.color); if(isS) DrawRectangleLinesEx({c.x+738,c.y-120,52,98},2.f,WHITE); }
            else if(l.laneIdx==3){ for(int s=0;s<5;s++) DrawRectangle(c.x-740,c.y+22+s*20,48,15,l.color); if(isS) DrawRectangleLinesEx({c.x-742,c.y+20,52,98},2.f,WHITE); }
        }
    }
public:
    void getActCnt(int lF[10], int& tA) const { for(int i=0;i<10;i++) lF[i]=0; tA=0; for(int i=0;i<6;i++) for(int j=0;j<4;j++){ for(Vehicle* c=grid[i]->lanes[j]->vQ->front;c;c=c->next) { if(c->originLane>=0&&c->originLane<10) lF[c->originLane]++; tA++; } for(const auto& ev:grid[i]->lanes[j]->exVs) { if(ev.originLane>=0&&ev.originLane<10) lF[ev.originLane]++; tA++; } for(const auto& ut:grid[i]->lanes[j]->aUTs) { if(ut.originLane>=0&&ut.originLane<10) lF[ut.originLane]++; tA++; } } }
    IntersectionSimulator() {
        grid[0]=new Intersection(0,0,200); grid[1]=new Intersection(1,1200,200); grid[2]=new Intersection(2,2400,200); grid[3]=new Intersection(3,0,1400); grid[4]=new Intersection(4,1200,1400); grid[5]=new Intersection(5,2400,1400);
        for(int r=0;r<2;r++) for(int c=0;c<3;c++){ int i=r*3+c; if(r>0) grid[i]->neighbors[0]=grid[(r-1)*3+c]; if(r<1) grid[i]->neighbors[1]=grid[(r+1)*3+c]; if(c<2) grid[i]->neighbors[2]=grid[r*3+c+1]; if(c>0) grid[i]->neighbors[3]=grid[r*3+c-1]; }
        vCounter=1000; fCounter=0; lastCr=-5400; reckAct=false; srand(time(0)); cam={0}; cam.target={1200.f,800.f}; cam.offset={600.f,400.f}; cam.rotation=0.f; cam.zoom=0.25f; cTime=TIME_MORNING; cWeath=WEATHER_SUMMER; sbOpen=true; sbProg=1.f;
        initUpcomingLanes(); updateTargets(); initParts(); genCity(); spdMult=1.f; tAcc=0.f;
    }
    ~IntersectionSimulator() { for(int i=0;i<6;i++) delete grid[i]; }
    void genTraf() {
        int lF[10]={0}, tA=0; getActCnt(lF,tA); float dH=getDHead(); bool sR=false; if(crashes.empty()&&!reckAct&&(fCounter-lastCr>5400)){ sR=true; reckAct=true; }
for(size_t i=0; i<uLanes.size(); i++){
            auto& l=uLanes[i]; if(l.targetPer30s>0){ float fI=1800.f/l.targetPer30s; l.spawnTimer+=1.f; if(l.spawnTimer>=fI && lF[i]<l.targetPer30s){ bool sC=true; for(Vehicle* c=grid[l.intId]->lanes[l.laneIdx]->vQ->front; c; c=c->next) if(c->pos>650.f-dH){ sC=false; break; }
            if(sC){ int dest=i<=2?3+(rand()%3):i==3||i==4?(rand()%2==0?2:5):i==5||i==6?(rand()%2==0?0:3):rand()%3; grid[l.intId]->lanes[l.laneIdx]->addVehicle(++vCounter,false,sR,dest,getTurnTypeForNeighbor(l.laneIdx,rand()%4),650.f,dH,(int)i); if(sR)sR=false; lF[i]++; tA++; l.spawnTimer-=fI; } } }
        }
    }
    void upLogic() {
        fCounter++; updateParts(); if(fCounter%180==135) for(int i=0;i<6;i++) for(int j=0;j<4;j++) if(grid[i]->lanes[j]->getState()==GREEN_STATE) grid[i]->lanes[j]->setState(YELLOW_STATE);
        if(fCounter%180==0) for(int i=0;i<6;i++) grid[i]->optimizeSignal(); genTraf(); std::vector<CarRect> aC; for(int i=0;i<6;i++) for(int j=0;j<4;j++) grid[i]->lanes[j]->getAllCarRects(j,aC);
        bool hasR=false; for(const auto& c:aC) if(c.isReckless) hasR=true; if(!hasR&&crashes.empty()) reckAct=false;
        for(size_t i=0;i<aC.size();i++) for(size_t j=i+1;j<aC.size();j++) if(aC[i].id!=aC[j].id && hypot(aC[i].rect.x-aC[j].rect.x,aC[i].rect.y-aC[j].rect.y)<38.f && (aC[i].isReckless||aC[j].isReckless) && crashes.empty()){ crashes.push_back({{(aC[i].rect.x+aC[j].rect.x)/2.f+20.f,(aC[i].rect.y+aC[j].rect.y)/2.f+20.f},1800,aC[i].id,aC[j].id}); lastCr=fCounter; reckAct=false; }
        std::vector<int> cErase; for(auto it=crashes.begin(); it!=crashes.end(); ){ it->framesLeft--; if(it->framesLeft<=0){ cErase.push_back(it->car1Id); cErase.push_back(it->car2Id); it=crashes.erase(it); } else ++it; }
        if(!cErase.empty()){ std::sort(cErase.begin(),cErase.end()); cErase.erase(std::unique(cErase.begin(),cErase.end()),cErase.end()); for(int i=0;i<6;i++) grid[i]->removeCrashedVehicles(cErase); }
        float wS=getWSpeed(), dH=getDHead(); for(int i=0;i<6;i++){ std::vector<CarRect> lC; for(int j=0;j<4;j++) grid[i]->lanes[j]->getAllCarRects(j,lC); grid[i]->update(lC,grid,wS,dH,crashes); }
    }
    void drwLogic() {
        BeginDrawing(); Color skyB[4]={{45,48,55,255},{52,58,65,255},{40,30,42,255},{14,16,22,255}}; ClearBackground(skyB[cTime]); BeginMode2D(cam); drawRds(); drawZebra(); for(int i=0;i<6;i++) grid[i]->drawStopLines();
        for(const auto& b:bldgs){ DrawRectangleRec({b.base.x+8.f,b.base.y+8.f,b.base.width,b.base.height},Fade(BLACK,0.4f)); DrawRectangleRec(b.base,b.col); DrawRectangleLinesEx(b.base,2.f,AdjustBrightness(b.col,-0.3f)); DrawRectangleRec({b.base.x+b.base.width*.3f,b.base.y+b.base.height*.3f,b.base.width*.4f,b.base.height*.4f},AdjustBrightness(b.col,0.1f)); DrawRectangleLinesEx({b.base.x+b.base.width*.3f,b.base.y+b.base.height*.3f,b.base.width*.4f,b.base.height*.4f},1.5f,AdjustBrightness(b.col,-0.1f)); if(cTime==TIME_NIGHT||cTime==TIME_EVENING){ DrawRectangle(b.base.x+8,b.base.y+8,8,8,Fade(YELLOW,0.7f)); DrawRectangle(b.base.x+b.base.width-16,b.base.y+8,8,8,Fade(YELLOW,0.7f)); DrawRectangle(b.base.x+8,b.base.y+b.base.height-16,8,8,Fade(YELLOW,0.7f)); DrawRectangle(b.base.x+b.base.width-16,b.base.y+b.base.height-16,8,8,Fade(YELLOW,0.7f)); } }
        for(int i=0;i<6;i++) for(int j=0;j<4;j++) grid[i]->lanes[j]->drawCars(j,cTime,crashes);
        for(const auto& c:crashes){ DrawCircle(c.pos.x,c.pos.y,48.f,RED); DrawCircle(c.pos.x,c.pos.y,42.f,Fade(WHITE,0.95f)); DrawLineEx({c.pos.x-26.f,c.pos.y-26.f},{c.pos.x+26.f,c.pos.y+26.f},16.f,RED); DrawLineEx({c.pos.x-26.f,c.pos.y+26.f},{c.pos.x+26.f,c.pos.y-26.f},16.f,RED); }
        for(int i=0;i<6;i++) for(int j=0;j<4;j++) grid[i]->lanes[j]->drawLights();
        if(cTime==TIME_NIGHT) DrawRectangle(-2500,-2000,7500,6500,Fade(BLACK,0.40f)); else if(cTime==TIME_EVENING) DrawRectangle(-2500,-2000,7500,6500,Fade(Color{255,110,40,255},0.08f)); else if(cTime==TIME_MORNING) DrawRectangle(-2500,-2000,7500,6500,Fade(Color{255,220,150,255},0.06f));
if(cWeath==WEATHER_RAINY) DrawRectangle(-2500,-2000,7500,6500,Fade(Color{40,60,90,255},0.15f)); else if(cWeath==WEATHER_WINTER) DrawRectangle(-2500,-2000,7500,6500,Fade(Color{180,210,240,255},0.12f)); else if(cWeath==WEATHER_AUTUMN) DrawRectangle(-2500,-2000,7500,6500,Fade(Color{200,110,30,255},0.08f)); else if(cWeath==WEATHER_SUMMER) DrawRectangle(-2500,-2000,7500,6500,Fade(Color{255,240,180,255},0.05f));
        EndMode2D(); int lF[10]={0}, tA=0; getActCnt(lF,tA);
        const char *tN[4]={"Morning","Afternoon","Evening","Night"}, *cT[4]={"08:30 AM","02:15 PM","06:45 PM","11:45 PM"}, *wN[5]={"Summer","Winter","Rainy","Spring","Autumn"}; Color tC[4]={{255,210,70,255},{100,205,255,255},{255,140,70,255},{175,135,255,255}}, wC[5]={{255,200,50,255},{170,220,255,255},{80,160,240,255},{120,230,140,255},{235,130,60,255}};
        for(const auto& p:parts){ if(cWeath==WEATHER_RAINY) DrawLine(p.x,p.y,p.x-2.5f,p.y+9.f,Fade(SKYBLUE,0.45f)); else if(cWeath==WEATHER_WINTER) DrawCircle(p.x,p.y,p.size*0.7f,Fade(WHITE,0.7f)); else if(cWeath==WEATHER_AUTUMN) DrawCircle(p.x,p.y,2.8f,Fade(ORANGE,0.6f)); }
        float hW=540.f, hX=(1200.f-hW)/2.f; DrawRectangle(hX,14.f,hW,52.f,Fade(BLACK,0.88f)); DrawRectangleLines(hX,14.f,hW,52.f,Fade(WHITE,0.25f));
        const char* l1=TextFormat("%s [%s]  |  Weather: %s",tN[cTime],cT[cTime],wN[cWeath]); DrawText(l1,hX+(hW-MeasureText(l1,15))/2,22,15,wC[cWeath]);
        int tCap=getCap(); const char* l2=TextFormat("Active Cars: %d / %d (%.0f%%)  |  Selected: %s (%s/30s)",tA,tCap,tCap>0?((float)tA/tCap)*100.f:0.f,uLanes[selLane].label,lStrs[selLane].c_str()); DrawText(l2,hX+(hW-MeasureText(l2,13))/2,43,13,LIGHTGRAY);
        float sX=-SB_W*(1.f-sbProg); DrawRectangle(sX,0,SB_W,800,Fade(Color{16,20,26,255},0.95f)); DrawRectangleLines(sX,0,SB_W,800,Fade(WHITE,0.15f)); DrawText("SIMULATOR DOCK",sX+18,18,15,WHITE);
        Rectangle cB={sX+SB_W-36.f,14.f,26.f,26.f}; DrawRectangleRec(cB,CheckCollisionPointRec(GetMousePosition(),cB)?Fade(WHITE,0.2f):Fade(WHITE,0.08f)); DrawRectangleLinesEx(cB,1.f,Fade(WHITE,0.3f)); DrawText("<",cB.x+8,cB.y+4,17,WHITE);
        DrawText("LANE ENTRY INJECTION",sX+18,48,11,GOLD); DrawText("Select Lane (Marked by Color):",sX+18,64,11,DARKGRAY);
        for(int i=0;i<10;i++){ Rectangle sw={sX+18.f+(i%5)*42.f,78.f+(i/5)*22.f,38.f,18.f}; bool iS=(i==selLane), iH=CheckCollisionPointRec(GetMousePosition(),sw); DrawRectangleRec(sw,iS?uLanes[i].color:Fade(uLanes[i].color,iH?0.8f:0.4f)); DrawRectangleLinesEx(sw,iS?2.f:1.f,iS?WHITE:Fade(WHITE,0.3f)); DrawText(uLanes[i].label,sw.x+(sw.width-MeasureText(uLanes[i].label,10))/2,sw.y+3,10,iS?BLACK:WHITE); }
        const auto& cL=uLanes[selLane]; DrawRectangle(sX+16,126,SB_W-32,92,Fade(BLACK,0.5f)); DrawRectangleLines(sX+16,126,SB_W-32,92,Fade(WHITE,0.12f)); DrawRectangle(sX+22,133,8,8,cL.color); DrawText(cL.fullName,sX+34,131,12,cL.color); DrawText(TextFormat("Nominal Capacity: %d cars",cL.maxCapacity),sX+22,146,11,LIGHTGRAY); DrawText("Enter Amount (Vehicles / 30s):",sX+22,161,11,WHITE);
        Rectangle iBx={sX+22.f,175.f,SB_W-44.f,24.f}; bool iH=CheckCollisionPointRec(GetMousePosition(),iBx); DrawRectangleRec(iBx,lInpAct?Color{24,30,40,255}:(iH?Color{20,25,35,255}:Color{12,15,20,255})); DrawRectangleLinesEx(iBx,lInpAct?1.5f:1.f,lInpAct?cL.color:(iH?WHITE:Fade(WHITE,0.3f))); const char* vS=lStrs[selLane].empty()?"0":lStrs[selLane].c_str(); DrawText(vS,iBx.x+8,iBx.y+5,14,lInpAct?WHITE:LIGHTGRAY);
        if(lInpAct&&((fCounter/30)%2==0)) DrawRectangle(iBx.x+9+MeasureText(vS,14),iBx.y+4,2,16,cL.color); DrawText(TextFormat("Active in City: %d / %d",lF[selLane],cL.targetPer30s),sX+22,203,11,lF[selLane]>=cL.targetPer30s?GREEN:LIGHTGRAY); DrawLine(sX+16,224,sX+SB_W-16,224,Fade(WHITE,0.15f));
        DrawText("WEATHER CONDITION",sX+18,230,11,LIGHTGRAY); const char* wL[5]={"[5] Summer","[6] Winter","[7] Rainy","[8] Spring","[9] Autumn"}; for(int w=0;w<5;w++){ Rectangle wB={sX+16,246.f+w*25.f,SB_W-32,23.f}; bool iS=cWeath==w, iHv=CheckCollisionPointRec(GetMousePosition(),wB); DrawRectangleRec(wB,iS?Fade(wC[w],0.35f):(iHv?Fade(WHITE,0.15f):Color{25,30,38,220})); DrawRectangleLinesEx(wB,iS?2.f:1.f,iS?wC[w]:(iHv?WHITE:Fade(WHITE,0.22f))); if(iS) DrawRectangle(wB.x+3,wB.y+3,3,wB.height-6,wC[w]); DrawText(wL[w],wB.x+14,wB.y+5,12,iS?wC[w]:WHITE); } DrawLine(sX+16,376,sX+SB_W-16,376,Fade(WHITE,0.15f));
        DrawText("TIME OF DAY",sX+18,382,11,LIGHTGRAY); const char* tL[4]={"[1] Morning","[2] Afternoon","[3] Evening","[4] Night"}; for(int t=0;t<4;t++){ Rectangle tB={sX+16,398.f+t*25.f,SB_W-32,23.f}; bool iS=cTime==t, iHv=CheckCollisionPointRec(GetMousePosition(),tB); DrawRectangleRec(tB,iS?Fade(tC[t],0.35f):(iHv?Fade(WHITE,0.15f):Color{25,30,38,220})); DrawRectangleLinesEx(tB,iS?2.f:1.f,iS?tC[t]:(iHv?WHITE:Fade(WHITE,0.22f))); if(iS) DrawRectangle(tB.x+3,tB.y+3,3,tB.height-6,tC[t]); DrawText(tL[t],tB.x+14,tB.y+5,12,iS?tC[t]:WHITE); } DrawLine(sX+16,504,sX+SB_W-16,504,Fade(WHITE,0.15f));
        if(crashes.empty()){ if(reckAct){ DrawText("CRASH STATUS",sX+18,510,11,ORANGE); Rectangle cC={sX+16,526.f,SB_W-32,48.f}; DrawRectangleRec(cC,Fade(BLACK,0.45f)); DrawRectangleLinesEx(cC,1.2f,Fade(ORANGE,0.6f)); DrawText("Active Crashes: 0",cC.x+10,cC.y+6,12,ORANGE); DrawText("Reckless Driver Detected!",cC.x+10,cC.y+22,10,LIGHTGRAY); DrawText("Impact pending...",cC.x+10,cC.y+34,10,LIGHTGRAY); }else{ DrawText("CRASH STATUS",sX+18,510,11,GREEN); Rectangle cC={sX+16,526.f,SB_W-32,44.f}; DrawRectangleRec(cC,Fade(BLACK,0.45f)); DrawRectangleLinesEx(cC,1.2f,Fade(GREEN,0.4f)); DrawText("Active Crashes: 0",cC.x+10,cC.y+7,12,GREEN); DrawText("Safe Protocol: Zero Overlap Enforced",cC.x+10,cC.y+26,9,LIGHTGRAY); } }else{ DrawText("CRASH STATUS",sX+18,510,11,RED); Rectangle cC={sX+16,526.f,SB_W-32,62.f}; DrawRectangleRec(cC,Fade(BLACK,0.55f)); DrawRectangleLinesEx(cC,1.2f,Fade(RED,0.8f)); DrawText(TextFormat("Active Crashes: %d",(int)crashes.size()),cC.x+10,cC.y+6,11,RED); DrawText(TextFormat("Loc: %s",getCrashLoc(crashes[0].pos,grid).c_str()),cC.x+10,cC.y+22,11,YELLOW); DrawText(TextFormat("Clear in: %ds (Cars #%d & #%d)",crashes[0].framesLeft/60,crashes[0].car1Id,crashes[0].car2Id),cC.x+10,cC.y+40,10,LIGHTGRAY); } DrawLine(sX+16,594,sX+SB_W-16,594,Fade(WHITE,0.15f));
        DrawText("SIM SPEED",sX+18,600,11,LIGHTGRAY); Rectangle spdB={sX+16,616.f,SB_W-32,28.f}; DrawRectangleRec(spdB,CheckCollisionPointRec(GetMousePosition(),spdB)?Fade(WHITE,0.2f):Color{25,30,38,220}); DrawRectangleLinesEx(spdB,1.2f,Fade(WHITE,0.4f)); const char* sD=TextFormat(spdMult==(int)spdMult?"Speed: %.0fx":"Speed: %.1fx",spdMult); DrawText(sD,spdB.x+(spdB.width-MeasureText(sD,14))/2,spdB.y+7,14,WHITE);
        DrawText("Click markings on road to pick lane",sX+18,654,10,DARKGRAY); DrawText("Click input box to type numbers",sX+18,668,10,DARKGRAY); DrawText("Cars enter smoothly across 30s",sX+18,682,10,DARKGRAY); DrawText("Keys: [C] Drawer | [T] Cycle Time",sX+18,696,10,DARKGRAY);
        if(sbProg<0.95f){ Rectangle eB={16.f,14.f,100.f,40.f}; DrawRectangleRec(eB,CheckCollisionPointRec(GetMousePosition(),eB)?Fade(WHITE,0.25f):Fade(BLACK,0.85f)); DrawRectangleLinesEx(eB,1.5f,Fade(WHITE,0.4f)); DrawLineEx({eB.x+14,eB.y+14},{eB.x+28,eB.y+14},2.f,WHITE); DrawLineEx({eB.x+14,eB.y+20},{eB.x+28,eB.y+20},2.f,WHITE); DrawLineEx({eB.x+14,eB.y+26},{eB.x+28,eB.y+26},2.f,WHITE); DrawText("Menu",eB.x+36,eB.y+12,16,WHITE); } EndDrawing();
    }
    void handleInputs() {
        Vector2 ms=GetMousePosition(); float sX=-SB_W*(1.f-sbProg); TimeOfDay pT=cTime; WeatherType pW=cWeath;
        if(lInpAct){ for(int k=GetCharPressed();k>0;k=GetCharPressed()) if(k>='0'&&k<='9'&&lStrs[selLane].length()<7){ if(lStrs[selLane]=="0") lStrs[selLane]=(char)k; else lStrs[selLane]+=(char)k; uLanes[selLane].targetPer30s=atoi(lStrs[selLane].c_str()); uLanes[selLane].isUserCustomized=true; } if(IsKeyPressed(KEY_BACKSPACE)&&!lStrs[selLane].empty()){ lStrs[selLane].pop_back(); uLanes[selLane].targetPer30s=lStrs[selLane].empty()?0:atoi(lStrs[selLane].c_str()); uLanes[selLane].isUserCustomized=true; } if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER)) lInpAct=false; }
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){ if(sbProg>0.5f&&CheckCollisionPointRec(ms,{sX+SB_W-36.f,14.f,26.f,26.f})){ sbOpen=false; lInpAct=false; } if(sbProg<0.5f&&CheckCollisionPointRec(ms,{16.f,14.f,100.f,40.f})) sbOpen=true; if(sbProg>0.6f){ for(int i=0;i<10;i++) if(CheckCollisionPointRec(ms,{sX+18.f+(i%5)*42.f,78.f+(i/5)*22.f,38.f,18.f})) selLane=i; if(CheckCollisionPointRec(ms,{sX+22.f,175.f,SB_W-44.f,24.f})){ lInpAct=true; if(lStrs[selLane]=="0") lStrs[selLane]=""; }else lInpAct=false; for(int w=0;w<5;w++) if(CheckCollisionPointRec(ms,{sX+16,246.f+w*25.f,SB_W-32,23.f})) cWeath=(WeatherType)w; for(int t=0;t<4;t++) if(CheckCollisionPointRec(ms,{sX+16,398.f+t*25.f,SB_W-32,23.f})) cTime=(TimeOfDay)t; if(CheckCollisionPointRec(ms,{sX+16,616.f,SB_W-32,28.f})) spdMult=(spdMult+=0.5f)>3.f?1.f:spdMult; }else{ Vector2 wM=GetScreenToWorld2D(ms,cam); for(size_t i=0;i<uLanes.size();i++){ Vector2 c=grid[uLanes[i].intId]->center; int l=uLanes[i].laneIdx; Rectangle mB=l==0?Rectangle{c.x-124.f,c.y-660.f,98.f,48.f}:l==1?Rectangle{c.x+16.f,c.y+660.f,98.f,48.f}:l==2?Rectangle{c.x+740.f,c.y-124.f,48.f,98.f}:Rectangle{c.x-740.f,c.y+16.f,48.f,98.f}; if(CheckCollisionPointRec(wM,mB)){ selLane=(int)i; sbOpen=true; break; } } } }
        if(!lInpAct){ if(IsKeyPressed(KEY_C)) sbOpen=!sbOpen; if(IsKeyPressed(KEY_ONE)) cTime=TIME_MORNING; if(IsKeyPressed(KEY_TWO)) cTime=TIME_AFTERNOON; if(IsKeyPressed(KEY_THREE)) cTime=TIME_EVENING; if(IsKeyPressed(KEY_FOUR)) cTime=TIME_NIGHT; if(IsKeyPressed(KEY_T)) cTime=(TimeOfDay)((cTime+1)%4); if(IsKeyPressed(KEY_FIVE)) cWeath=WEATHER_SUMMER; if(IsKeyPressed(KEY_SIX)) cWeath=WEATHER_WINTER; if(IsKeyPressed(KEY_SEVEN)) cWeath=WEATHER_RAINY; if(IsKeyPressed(KEY_EIGHT)) cWeath=WEATHER_SPRING; if(IsKeyPressed(KEY_NINE)) cWeath=WEATHER_AUTUMN; }
        if(pT!=cTime||pW!=cWeath) updateTargets(); float tg=sbOpen?1.f:0.f; sbProg+=(tg-sbProg)*0.22f; if(fabs(sbProg-tg)<0.005f) sbProg=tg;
    }
    void runFrame() { handleInputs(); tAcc+=spdMult; while(tAcc>=1.f){ upLogic(); tAcc-=1.f; } drwLogic(); }
};
int main() { InitWindow(1200, 800, "Dynamic Traffic Signal Optimization Simulator"); SetTargetFPS(60); IntersectionSimulator s; while(!WindowShouldClose()) s.runFrame(); CloseWindow(); return 0; }
