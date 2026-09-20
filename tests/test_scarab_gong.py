from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
temporary = TemporaryDirectory(prefix='aq-gong-runtime-')
w = Path(temporary.name)
p = Path(__file__).resolve().parents[1] / 'src'
pre=r'''
#include <array>
#include <string>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <cassert>
#include <algorithm>
#include <thread>
#include <iostream>
#include <ctime>
#include <chrono>
#define LOG_WARN(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define private public
#include "AQWarEffort.h"
#undef private
#define LOG_ERROR(...) ((void)0)
#define LOG_INFO(...) ((void)0)
struct ObjectGuid {uint64 id=0; uint32 GetCounter()const{return id;}};
enum GOState {GO_STATE_ACTIVE,GO_STATE_READY};
constexpr uint32 GO_FLAG_INTERACT_COND=4,QUEST_REWARD_CHOICES_COUNT=6;
constexpr uint32 CMSG_QUESTGIVER_CHOOSE_REWARD=398,SERVERHOOK_CAN_PACKET_RECEIVE=1;
class GameObject {public:
 uint32 spawn,entry,map=1,script=9,mask=1; GOState state=GO_STATE_READY;bool collision=true;std::vector<uint32> sounds;
 uint32 GetMapId(){return map;}uint32 GetSpawnId(){return spawn;}uint32 GetEntry(){return entry;}uint32 GetScriptId(){return script;}
 uint32 GetPhaseMask(){return mask;}GOState GetGoState(){return state;}
 void SetPhaseMask(uint32 v,bool){mask=v;}void SetGoState(GOState v){state=v;}void EnableCollision(bool v){collision=v;}
 void PlayDistanceSound(uint32 v){sounds.push_back(v);}void RemoveGameObjectFlag(uint32){}
};
struct Map {std::multimap<uint32,GameObject*> objects; void LoadGrid(float,float){}auto& GetGameObjectBySpawnIdStore(){return objects;}
 GameObject* GetGameObject(ObjectGuid g){auto i=objects.find(g.id);return i==objects.end()?nullptr:i->second;}} worldMap;
struct MapMgr {Map* CreateBaseMap(uint32){return &worldMap;}} mapMgr;auto sMapMgr=&mapMgr;
struct ObjectMgr {uint32 GetScriptId(char const*){return 9;}} objectMgr;auto sObjectMgr=&objectMgr;
class Player;
class WorldSession {public:Player* player=nullptr;Player* GetPlayer(){return player;}};
class Player {public:uint32 id;bool rewarded=false;WorldSession session;struct Talk{void SendCloseGossip(){}} talk;Talk* PlayerTalkClass=&talk;
 Player(uint32 v):id(v){session.player=this;}ObjectGuid GetGUID(){return {id};}bool GetQuestRewardStatus(uint32){return rewarded;}
 Map* GetMap(){return &worldMap;}WorldSession* GetSession(){return &session;}};
struct ChatHandler {ChatHandler(WorldSession*){}void SendSysMessage(char const*){}};
struct WorldPacket {uint32 opcode=CMSG_QUESTGIVER_CHOOSE_REWARD,quest=8743,reward=0;ObjectGuid guid;size_t pos=0;
 uint32 GetOpcode()const{return opcode;}size_t size()const{return 16;}size_t rpos()const{return pos;}
 WorldPacket& operator>>(ObjectGuid& g){g=guid;pos+=8;return *this;}
 WorldPacket& operator>>(uint32& v){v=pos==8?quest:reward;pos+=4;return *this;}};
class Quest {public:uint32 GetQuestId()const{return 8743;}};
class ServerScript {public:ServerScript(char const*,std::initializer_list<uint32>){}virtual bool CanPacketReceive(WorldSession*,WorldPacket const&){return true;}};
class AllGameObjectScript {public:AllGameObjectScript(char const*){}virtual void OnGameObjectAddWorld(GameObject*){}virtual void OnGameObjectUpdate(GameObject*,uint32){}
 virtual bool CanGameObjectQuestReward(Player*,GameObject*,Quest const*,uint32){return false;}};
class GameObjectScript {public:GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct Field {uint64 value;template<class T>T Get(){return T(value);}};
struct Result {std::vector<Field> fields;Field* Fetch(){return fields.data();} Field& operator[](size_t i){return fields.at(i);}};
using QueryResult=std::shared_ptr<Result>;
QueryResult row(std::initializer_list<uint64> v){auto r=std::make_shared<Result>();for(auto n:v)r->fields.push_back({n});return r;}
struct Transaction { template<class... T> void Append(char const*, T...){} };
using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
struct DB {
 CharacterDatabaseTransaction BeginTransaction(){return std::make_shared<Transaction>();}
 void DirectCommitTransaction(CharacterDatabaseTransaction){assert(false);}

 bool schema=true,intent=false,accepted=false,failRead=false,failWrite=false;uint32 player=0;uint64 epoch=0;
 std::set<uint32> rewarded;AQWarEffort::Campaign campaign;int commits=0;
 template<class... T>QueryResult Query(char const* query,T...){std::string q(query);if(!schema||failRead)return {};
 if(q.find("COUNT(*)")!=q.npos)return row({intent?1u:0u});if(!intent)return {};
 if(q.find("SELECT player_guid, ready_started_at")!=q.npos)return row({player,epoch,accepted?1u:0u});
 if(q.find("SELECT player_guid, accepted")!=q.npos)return row({player,accepted?1u:0u});
 if(q.find("SELECT g.ready_started_at")!=q.npos)return row({epoch,rewarded.count(player)});
 if(q.find("SELECT accepted")!=q.npos)return row({accepted?1u:0u});assert(false);return {};}
 template<class... T>void DirectExecute(char const* query,T...args){if(failWrite||!schema)return;std::string q(query);std::vector<uint64> a{uint64(args)...};
 if(q.starts_with("INSERT")){if(campaign.Phase==AQ_PHASE_READY&&campaign.PhaseStartedAt==a[3]&&!rewarded.count(a[0])){intent=true;accepted=false;player=a[0];epoch=a[3];}}
 else if(q.starts_with("DELETE")){if(!accepted)intent=false;}
 else if(q.starts_with("UPDATE")){if(intent&&!accepted&&campaign.Phase==AQ_PHASE_READY&&campaign.PhaseStartedAt==epoch&&rewarded.count(player)){
 accepted=true;campaign.Phase=AQ_PHASE_TEN_HOUR_WAR;campaign.PhaseStartedAt=campaign.GongRungAt=a[2];commits++;}}
 else assert(false);}
} CharacterDatabase;
namespace AQWarEffort {
Manager& Manager::Instance(){static Manager m;return m;}
bool Manager::ReadCampaign(Campaign& c)const{if(CharacterDatabase.failRead)return false;c=CharacterDatabase.campaign;return true;}
void Manager::SyncCollectionEvent(){}
char const* Manager::PhaseName(AQCampaignPhase){return "test";}
}
'''
body='\n'.join('\n'.join(l for l in (p/f).read_text().splitlines() if not l.startswith('#include')) for f in ['AQScarabWall.cpp','AQScarabGong.cpp'])
manager=(p/'mod_aq_war_effort.cpp').read_text()
body+='\nnamespace AQWarEffort {\n'+manager[manager.index('void Manager::EnterPhase('):manager.index('bool Manager::SetPhase(')]+'\n}\n'
post=r'''
int main(){using namespace AQWarEffort;auto& m=Manager::Instance();
 GameObject gong{9100717,180717},roots{9100147,176147},runes{9100148,176148},gate{9100146,176146},cinematic{0,176146};
 for(auto go:{&gong,&roots,&runes,&gate,&cinematic})worldMap.objects.emplace(go->spawn,go);
 auto reset=[&](){CharacterDatabase=DB{};CharacterDatabase.campaign.Phase=AQ_PHASE_READY;CharacterDatabase.campaign.PhaseStartedAt=100;
 CharacterDatabase.campaign.Contributions[0][0]=12345;CharacterDatabase.campaign.OpenedAt=77;
 m._campaign=CharacterDatabase.campaign;m._loaded=m._enabled=true;m.InitializeGong();roots.sounds.clear();runes.sounds.clear();gate.sounds.clear();m.SyncWall();};
 Player a(10),b(20);Quest quest;aq_scarab_gong_packets packets;aq_scarab_gong_rewards observer;
 WorldPacket packet;packet.guid={9100717};
 reset();for(auto phase:{AQ_PHASE_DISABLED,AQ_PHASE_WAR_EFFORT,AQ_PHASE_TEN_HOUR_WAR,AQ_PHASE_OPEN}){m._campaign.Phase=phase;assert(!packets.CanPacketReceive(&a.session,packet));assert(!CharacterDatabase.intent);}
 reset();m._enabled=false;assert(!m.PrepareGong(&a,&gong));m._enabled=true;m._loaded=false;assert(!m.PrepareGong(&a,&gong));
 reset();GameObject wrong{9100717,180717};wrong.script=8;assert(!m.PrepareGong(&a,&wrong));a.rewarded=true;assert(!m.PrepareGong(&a,&gong));a.rewarded=false;
 reset();assert(packets.CanPacketReceive(&a.session,packet));assert(packet.rpos()==0);assert(!m.PrepareGong(&b,&gong));
 // Stock rejection: no observer, no durable reward. Reservation released only at end of update.
 m.UpdateGong(1);assert(!m._gongPending&&!CharacterDatabase.intent&&m.GongAvailable());
 assert(m.PrepareGong(&a,&gong));a.rewarded=true;assert(!observer.CanGameObjectQuestReward(&a,&gong,&quest,0));
 m.UpdateGong(1);assert(m._gongPending&&m.GetPhase()==AQ_PHASE_READY); // asynchronous save still pending
 CharacterDatabase.rewarded.insert(a.id);m.UpdateGong(1);assert(m.GetPhase()==AQ_PHASE_TEN_HOUR_WAR&&CharacterDatabase.commits==1);
 assert(m._campaign.Contributions[0][0]==12345&&m._campaign.OpenedAt==77&&m._campaign.GongRungAt>100&&m._campaign.GongRungAt==m._campaign.PhaseStartedAt);
 assert(roots.state==GO_STATE_ACTIVE&&!roots.collision&&roots.mask==1&&runes.collision&&gate.collision);
 assert(roots.sounds==std::vector<uint32>{7114});
 GameObject reloadedRoots{9100147,176147};aq_scarab_wall_objects wallHook;wallHook.OnGameObjectAddWorld(&reloadedRoots);
 assert(reloadedRoots.state==GO_STATE_ACTIVE&&!reloadedRoots.collision&&reloadedRoots.mask==1);
 m.UpdateGong(3999);assert(runes.sounds.empty());m.UpdateGong(1);
 wallHook.OnGameObjectUpdate(&reloadedRoots,1);assert(reloadedRoots.mask==0&&!reloadedRoots.collision);
 assert(roots.mask==0&&runes.state==GO_STATE_ACTIVE&&!runes.collision&&gate.collision&&runes.sounds==std::vector<uint32>{7116});
 m.UpdateGong(4000);assert(runes.mask==0&&gate.state==GO_STATE_ACTIVE&&gate.sounds==std::vector<uint32>{7115});
 m.UpdateGong(4000);assert(!m.WallCeremonyActive()&&gate.mask==0&&!gate.collision&&cinematic.mask==1&&cinematic.collision);
 m.UpdateGong(99999);assert(CharacterDatabase.commits==1&&gate.sounds.size()==1&&!m.PrepareGong(&b,&gong));
 auto settled=m._campaign;observer.CanGameObjectQuestReward(&a,&gong,&quest,0);m.UpdateGong(1);
 assert(m._campaign==settled&&!m._gongPending&&!m._gongObserved);
 // Crash after durable reward, before observation/acceptance: recover once without sounds.
 reset();a.rewarded=false;assert(m.PrepareGong(&a,&gong));CharacterDatabase.rewarded.insert(a.id);m.InitializeGong();
 assert(m.GetPhase()==AQ_PHASE_TEN_HOUR_WAR&&!m.WallCeremonyActive()&&roots.sounds.empty()&&gate.mask==0);
 m.InitializeGong();assert(CharacterDatabase.commits==1);
 // Crash before reward durability: safe retry; disabled startup preserves journal.
 reset();assert(m.PrepareGong(&a,&gong));m._enabled=false;m.InitializeGong();assert(CharacterDatabase.intent&&m._gongPending);
 m._enabled=true;m.InitializeGong();assert(!CharacterDatabase.intent&&m.GongAvailable());
 // Failed acceptance retains durable intent for restart recovery.
 assert(m.PrepareGong(&a,&gong));a.rewarded=true;observer.CanGameObjectQuestReward(&a,&gong,&quest,0);CharacterDatabase.rewarded.insert(a.id);
 CharacterDatabase.failWrite=true;m.UpdateGong(1);assert(m._gongPending&&!m._gongHealthy&&CharacterDatabase.intent&&m.GetPhase()==AQ_PHASE_READY);
 CharacterDatabase.failWrite=false;m._loaded=true;m.InitializeGong();assert(m.GetPhase()==AQ_PHASE_TEN_HOUR_WAR&&!m.WallCeremonyActive());
 // Exactly one concurrent packet reserves the campaign, even before either stock reward executes.
 reset();a.rewarded=false;bool first=false,second=false;std::thread t1([&]{first=m.PrepareGong(&a,&gong);});std::thread t2([&]{second=m.PrepareGong(&b,&gong);});t1.join();t2.join();assert(first!=second);
 // Unknown database writes and wrong epoch fail closed.
 reset();CharacterDatabase.failWrite=true;assert(!m.PrepareGong(&a,&gong));assert(m._gongPending&&!m._gongHealthy);
 reset();assert(m.PrepareGong(&a,&gong));CharacterDatabase.epoch=200;m.UpdateGong(1);assert(!m._gongHealthy&&m._gongPending);
 // Admin phase entry must not fabricate a gong timestamp or reset opened_at.
 Campaign admin;admin.GongRungAt=55;admin.OpenedAt=66;Manager::EnterPhase(admin,AQ_PHASE_TEN_HOUR_WAR);
 assert(admin.GongRungAt==55&&admin.OpenedAt==66&&admin.Phase==AQ_PHASE_TEN_HOUR_WAR);
 // Missing wall object skips presentation but keeps the accepted/open campaign.
 reset();worldMap.objects.erase(9100148);a.rewarded=false;assert(m.PrepareGong(&a,&gong));a.rewarded=true;
 observer.CanGameObjectQuestReward(&a,&gong,&quest,0);CharacterDatabase.rewarded.insert(a.id);m.UpdateGong(1);
 assert(m.GetPhase()==AQ_PHASE_TEN_HOUR_WAR&&!m.WallCeremonyActive()&&gate.mask==0&&roots.mask==0&&roots.sounds.empty());
 std::cout<<"PASS actual gong/wall source: phase and ownership guards, concurrent reservation, stock rejection, asynchronous durability, recovery, write failure, ceremony stages/sounds, no replay, preserved data\n";
}
'''
(w/'EventMap.h').write_text("""#pragma once
#include <map>
#include <chrono>
using Milliseconds=std::chrono::milliseconds;
class EventMap { unsigned long long elapsed=0;std::multimap<unsigned long long,unsigned> events;
public:void Reset(){elapsed=0;events.clear();}void Update(unsigned diff){elapsed+=diff;}
void ScheduleEvent(unsigned id,Milliseconds delay){events.emplace(elapsed+delay.count(),id);}
unsigned ExecuteEvent(){auto i=events.begin();if(i==events.end()||i->first>elapsed)return 0;auto id=i->second;events.erase(i);return id;}};
""")
(w/'Define.h').write_text('#pragma once\n#include <cstdint>\nusing uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;\n')
(w/'runtime_test.cpp').write_text(pre+body+post)
subprocess.run(['g++','-std=c++20','-pthread','-I'+str(w),'-I'+str(p),str(w/'runtime_test.cpp'),'-o',str(w/'runtime_test')],check=True)
subprocess.run([str(w/'runtime_test')],check=True)
