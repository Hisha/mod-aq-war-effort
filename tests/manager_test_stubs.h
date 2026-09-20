
#include <cassert>
#include <ctime>
#include <limits>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <chrono>
std::time_t testNow = 1000000;
#include <map>
#include <vector>
#include <memory>
#define private public
#include "AQWarEffort.h"
#include "AQWarContent.h"
#undef private
#define LOG_ERROR(...) (++testErrorLogs)
int testErrorLogs=0, testInfoLogs=0;
#define LOG_INFO(...) (++testInfoLogs)
#define LOG_WARN(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#include <functional>
#include <charconv>
constexpr uint8 TEAM_ALLIANCE=0, TEAM_HORDE=1;
class Player { public: uint8 faction=0; uint8 GetTeamId(){return faction;} void* GetSession(){return nullptr;} };
int notices=0;
struct ChatHandler { ChatHandler(void*){} void SendSysMessage(char const*){++notices;} template<class... A> void PSendSysMessage(char const*, A...){++notices;} };
struct Config { bool enabled=true; uint32 id=1; uint32 goal=40; std::string duration="36000"; std::string rawGoal; std::vector<std::string> keys; auto GetKeysByString(std::string const&){return keys;} template<class T> T GetOption(char const* key,T) { std::string k=key; if constexpr (std::is_same_v<T,std::string>) return k=="AQWarEffort.TenHourWar.Duration" ? duration : (rawGoal.empty()?std::to_string(goal):rawGoal); else return T(k=="AQWarEffort.Enable"? enabled : k=="AQWarEffort.Id"? id : goal); } } config;
auto sConfigMgr=&config;
constexpr uint8 QUEST_ITEM_OBJECTIVES_COUNT=6;
class Quest { public: uint32 id=0; uint32 RequiredItemId[6]{}; uint32 RequiredItemCount[6]{}; uint32 GetQuestId() const {return id;} };
constexpr uint32 GAMEOBJECT_TYPE_DOOR=0, GO_FLAG_NOT_SELECTABLE=16;
struct GameObjectTemplate { uint32 type=0, displayId=6573, ScriptId=0, linked=0; std::string AIName; uint32 GetLinkedGameObjectEntry()const{return linked;} };
struct ObjectMgr { bool templateMissing=false; GameObjectTemplate crystal;
 GameObjectTemplate const* GetGameObjectTemplate(uint32 entry){assert(entry==180810);return templateMissing?nullptr:&crystal;} std::map<uint32,Quest> quests; Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;} } objectMgr;
auto sObjectMgr=&objectMgr;
struct Event { bool isValid()const{return true;} };
struct Events {
 bool active=false; int starts=0,stops=0; std::vector<Event> entries=std::vector<Event>(23); std::function<void()> changed;
 auto const& GetEventMap(){return entries;} bool IsActiveEvent(uint16_t){return active;}
 void StartEvent(uint16_t id,bool overwrite){assert(id==22&&!overwrite);active=true;++starts;if(changed)changed();}
 void StopEvent(uint16_t id,bool overwrite){assert(id==22&&!overwrite);active=false;++stops;if(changed)changed();}
} events;
auto sGameEventMgr=&events;
struct Field { uint64 value; template<class T> T Get()const{return T(value);} };
struct Result { std::vector<std::vector<Field>> rows; size_t pos=0; Field* Fetch(){return rows[pos].data();} Field const& operator[](size_t i)const{return rows[pos][i];} size_t GetRowCount(){return rows.size();} bool NextRow(){return ++pos<rows.size();} };
using QueryResult=std::shared_ptr<Result>;
struct Transaction { std::vector<std::vector<uint64>> updates; template<class... A> void Append(char const*,A... a){updates.push_back({uint64(a)...});} };
using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
struct DB {
 std::map<uint32,AQWarEffort::Campaign> rows;
 std::map<uint32,bool> hasCampaign;
 bool failWrite=false, failRead=false; uint32 version=2;
 int commits=0;
 template<class... A> void DirectExecute(char const* sql,A... a){
  if(failWrite)return;
  std::vector<uint64> v={uint64(a)...};
  if(std::string(sql).find("INSERT INTO aq_war_effort_campaign")!=std::string::npos){
   if(!hasCampaign[v[0]]){auto& c=rows[v[0]];c.Phase=AQCampaignPhase(v[1]);c.PhaseStartedAt=v[2];hasCampaign[v[0]]=true;}
  } else rows.try_emplace(v[0]);
 }
 QueryResult Query(char const*) { if(failRead)return nullptr; auto r=std::make_shared<Result>();r->rows={{{version}}};return r; }
 QueryResult Query(char const* sql,uint32 id){
  if(failRead || !rows.count(id))return nullptr;
  bool join=std::string(sql).find("JOIN")!=std::string::npos;
  if(join&&!hasCampaign[id])return nullptr;
  auto r=std::make_shared<Result>();auto& c=rows[id];
  for(uint8 f=0;f<2;++f){std::vector<Field> row;
   if(join)row={{uint64(c.Phase)},{c.PhaseStartedAt},{c.GongRungAt},{c.OpenedAt}};
   row.push_back({f});for(auto count:c.Contributions[f])row.push_back({count});r->rows.push_back(row);
  }return r;
 }
 CharacterDatabaseTransaction BeginTransaction(){return std::make_shared<Transaction>();}
 void DirectCommitTransaction(CharacterDatabaseTransaction t){
  ++commits;if(failWrite)return;
  for(auto& v:t->updates){if(v.size()==17){auto& c=rows[v[15]];for(size_t i=0;i<15;++i)c.Contributions[v[16]][i]=v[i];}
  else{auto& c=rows[v[4]];c.Phase=AQCampaignPhase(v[0]);c.PhaseStartedAt=v[1];c.GongRungAt=v[2];c.OpenedAt=v[3];}}
 }
} CharacterDatabase;


struct GameObject {
 ObjectGuid guid; uint32 phase=1, flags=0; bool collision=true, active=false, pendingDelete=false;
 ObjectGuid GetGUID()const{return guid;}
 void Delete(){pendingDelete=true;collision=true;}
 void SetPhaseMask(uint32 p,bool){phase=p;}
 void EnableCollision(bool enabled){collision=enabled;}
 void setActive(bool enabled){active=enabled;}
 void SetGameObjectFlag(uint32 f){flags|=f;}
};
struct Map {
 std::map<uint64,GameObject> objects; uint64 next=1; int loads=0, attempts=0, creates=0; bool fail=false;
 GameObject* GetGameObject(ObjectGuid id){auto i=objects.find(id.value);return i==objects.end()?nullptr:&i->second;}
 void LoadGrid(float x,float y){assert(x==-8088.0f&&y==1530.0f);++loads;}
 GameObject* SummonGameObject(uint32 entry,Position const& p,float r0,float r1,float r2,float r3,uint32 duration){
  ++attempts;assert(entry==180810&&p.x==-8088.0f&&p.y==1530.0f&&p.z==2.61f&&p.o==0);
  assert(r0==0&&r1==0&&r2==0&&r3==1&&duration==0);
  if(fail)return nullptr;
  ++creates;auto& go=objects[next];go.guid.value=next++;return &go;
 }
 void Flush(){for(auto i=objects.begin();i!=objects.end();)if(i->second.pendingDelete)i=objects.erase(i);else ++i;}
 size_t Visible()const{size_t n=0;for(auto const& [id,go]:objects)if(go.phase)n++;return n;}
} contentMap;
struct MapMgr {
 bool present=true,failCreate=false;
 Map* FindMap(uint32 map,uint32 instance){assert(map==1&&instance==0);return present?&contentMap:nullptr;}
 Map* CreateBaseMap(uint32 map){assert(map==1);if(failCreate)return nullptr;present=true;return &contentMap;}
} mapMgr;
auto sMapMgr=&mapMgr;
