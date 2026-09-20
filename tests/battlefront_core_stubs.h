#pragma once
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <vector>
#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#define private public
#include "AQWarContent.h"
#undef private

inline unsigned InfoLogs = 0, ErrorLogs = 0;
#define LOG_INFO(...) (++InfoLogs)
#define LOG_ERROR(...) (++ErrorLogs)
constexpr uint32 GAMEOBJECT_TYPE_DOOR = 0, GO_FLAG_NOT_SELECTABLE = 16;
struct GameObjectTemplate
{
    uint32 type = 0, displayId = 6573, ScriptId = 0;
    std::string AIName;
    uint32 GetLinkedGameObjectEntry() const { return 0; }
};
struct CreatureTemplate {};
struct ObjectMgr
{
    GameObjectTemplate Crystal;
    CreatureTemplate CreatureData;
    std::set<uint32> Missing;
    GameObjectTemplate const* GetGameObjectTemplate(uint32 entry)
    { assert(entry == 180810); return Missing.contains(entry) ? nullptr : &Crystal; }
    CreatureTemplate const* GetCreatureTemplate(uint32 entry)
    { return Missing.contains(entry) ? nullptr : &CreatureData; }
} inline ObjectManager;
inline auto sObjectMgr = &ObjectManager;
class Creature;
class Unit
{
public:
    virtual ~Unit() = default;
    virtual Creature* ToCreature() { return nullptr; }
};
class Creature : public Unit
{
public:
    uint32 Entry = 0, Phase = 1;
    ObjectGuid Guid;
    Position Location{};
    bool Active = false, Removed = false;
    Creature* ToCreature() override { return this; }
    uint32 GetEntry() const { return Entry; }
    ObjectGuid GetGUID() const { return Guid; }
    void SetPhaseMask(uint32 phase, bool) { Phase = phase; }
    void DespawnOrUnsummon() { Removed = true; }
    void setActive(bool active) { Active = active; }
};
class TempSummon : public Creature {};
class GameObject
{
public:
    ObjectGuid Guid;
    uint32 Phase = 1, Flags = 0;
    bool Removed = false, Collision = true, Active = false;
    ObjectGuid GetGUID() const { return Guid; }
    void Delete() { Removed = true; }
    void SetPhaseMask(uint32 phase, bool) { Phase = phase; }
    void EnableCollision(bool value) { Collision = value; }
    void setActive(bool value) { Active = value; }
    void SetGameObjectFlag(uint32 flag) { Flags |= flag; }
};
class Map
{
public:
    uint64 Next = 1;
    unsigned CreatureCreates = 0, CrystalCreates = 0;
    std::map<uint64, TempSummon> Creatures;
    std::map<uint64, GameObject> Objects;
    std::set<uint32> FailSummon;
    void LoadGrid(float, float) {}
    Creature* GetCreature(ObjectGuid guid)
    { auto it = Creatures.find(guid.Value); return it == Creatures.end() ? nullptr : &it->second; }
    GameObject* GetGameObject(ObjectGuid guid)
    { auto it = Objects.find(guid.Value); return it == Objects.end() ? nullptr : &it->second; }
    TempSummon* SummonCreature(uint32 entry, Position const& location)
    {
        if (FailSummon.contains(entry)) return nullptr;
        auto& actor = Creatures[Next];
        actor.Guid.Value = Next++; actor.Entry = entry; actor.Location = location;
        ++CreatureCreates;
        return &actor;
    }
    GameObject* SummonGameObject(uint32 entry, Position const& location, float a, float b, float c, float d, uint32 time)
    {
        assert(entry == 180810 && location.x == -8088 && location.y == 1530);
        assert(a == 0 && b == 0 && c == 0 && d == 1 && time == 0);
        auto& go = Objects[Next]; go.Guid.Value = Next++; ++CrystalCreates; return &go;
    }
    void Flush()
    {
        std::erase_if(Creatures, [](auto const& item) { return item.second.Removed; });
        std::erase_if(Objects, [](auto const& item) { return item.second.Removed; });
    }
    unsigned Count(uint32 entry) const
    {
        unsigned count = 0;
        for (auto const& [guid, actor] : Creatures)
            if (actor.Entry == entry && actor.Phase && !actor.Removed) ++count;
        return count;
    }
} inline TestMap;
struct MapMgr
{
    Map* FindMap(uint32 id, uint32 instance) { assert(id == 1 && !instance); return &TestMap; }
    Map* CreateBaseMap(uint32 id) { assert(id == 1); return &TestMap; }
} inline MapManager;
inline auto sMapMgr = &MapManager;
struct Field { uint64 Value; template<class T> T Get() const { return T(Value); } };
struct Result { Field Value; Field* Fetch() { return &Value; } };
using QueryResult = std::shared_ptr<Result>;
using BossKey = std::tuple<uint32, uint64, uint32>;
struct DB
{
    std::map<BossKey, uint64> Kills;
    std::set<uint32> FailRead, FailWrite;
    unsigned Reads = 0, Writes = 0;
    QueryResult Query(char const* sql, uint32 campaign, uint64 epoch, uint32 boss)
    {
        assert(std::string(sql).starts_with("SELECT COUNT(*) FROM aq_war_effort_boss_kill"));
        ++Reads;
        if (FailRead.contains(boss)) return nullptr;
        return std::make_shared<Result>(Result{{uint64(Kills.contains({campaign, epoch, boss}))}});
    }
    void DirectExecute(char const* sql, uint32 campaign, uint64 epoch, uint32 boss, uint64 killedAt)
    {
        assert(std::string(sql).find("ON DUPLICATE KEY UPDATE boss_id = boss_id") != std::string::npos);
        ++Writes;
        if (!FailWrite.contains(boss)) Kills.try_emplace(BossKey{campaign, epoch, boss}, killedAt);
    }
} inline CharacterDatabase;
constexpr int UNITHOOK_ON_UNIT_DEATH = 0;
struct UnitScript
{
    UnitScript(char const*, bool, std::initializer_list<int>) {}
    virtual ~UnitScript() = default;
    virtual void OnUnitDeath(Unit*, Unit*) {}
};
inline AQWarEffort::WarContentState TestState;
namespace AQWarEffort
{
    Manager& Manager::Instance() { static Manager instance; return instance; }
    WarContentState Manager::GetWarContentState() const { return TestState; }
    char const* Manager::PhaseName(AQCampaignPhase) { return "test phase"; }
}
