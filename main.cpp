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
