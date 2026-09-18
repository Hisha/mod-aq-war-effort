/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * Crystal asset referenced by AzerothCore mod-war-effort warevent.sql.
 * Proof placement uses an AzerothCore zone_silithus.cpp stock battle-scene position.
 */
#include "AQWarContent.h"
#include "AQBattlefrontStage.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectMgr.h"
#include "QueryResult.h"
#include "TemporarySummon.h"
#include "UnitScript.h"
#include <ctime>
#include <iterator>

namespace AQWarEffort
{
namespace
{
    constexpr uint32 ContentMap = 1;
    constexpr uint32 CrystalEntry = 180810;
    constexpr uint32 CrystalDisplay = 6573;
    // Stock quest 8519 SpawnLocation[37], outside the gate. This is a new proof
    // placement for the crystal, not a historical crystal spawn from warevent.sql.
    Position const CrystalPosition{ -8088.0f, 1530.0f, 2.61f, 0.0f };
    constexpr auto RetryDelay = std::chrono::seconds(5);

    // The old warevent.sql places 15742 at the Hive'Ashi mouth. Its later
    // waypoint coordinates jump across the continent, so none are imported.
    // Slots are a small scene, not database spawn GUIDs or a wave backlog.
    struct SpawnDefinition
    {
        uint32 Entry;
        Position Location;
        uint8 FirstStage;
    };
    constexpr uint32 Drone = 15421;
    constexpr uint32 Warbringer = 15758;
    constexpr uint32 AshiColossus = BOSS_COLOSSUS_ASHI;
    SpawnDefinition const AshiSpawns[] =
    {
        { Drone, { -6497.20f, 1021.79f, 0.38f, 4.00f }, 1 },
        { Drone, { -6540.54f, 985.64f, 0.38f, 3.59f }, 1 },
        { Drone, { -6704.02f, 899.43f, -1.39f, 4.24f }, 2 },
        { Warbringer, { -6610.06f, 924.65f, 0.37f, 3.08f }, 2 },
        { Warbringer, { -6669.62f, 922.71f, -0.69f, 3.68f }, 3 },
        { AshiColossus, { -6458.70f, 1076.01f, -2.90f, 4.05f }, 4 }
    };

}

WarContentController& WarContentController::Instance()
{
    static WarContentController instance;
    return instance;
}

bool WarContentController::PersistBossKill(uint32 campaignId, uint64 origin, uint32 entry, uint64 killedAt)
{
    // The natural key makes duplicate callbacks/retries harmless. DirectExecute
    // is synchronous; its void API requires a read-back before claiming success.
    CharacterDatabase.DirectExecute(
        "INSERT INTO aq_war_effort_boss_kill (campaign_id, war_started_at, boss_id, killed_at) "
        "VALUES ({}, {}, {}, {}) ON DUPLICATE KEY UPDATE boss_id = boss_id",
        campaignId, origin, entry, killedAt);
    QueryResult result = CharacterDatabase.Query(
        "SELECT COUNT(*) FROM aq_war_effort_boss_kill "
        "WHERE campaign_id = {} AND war_started_at = {} AND boss_id = {}",
        campaignId, origin, entry);
    if (!result)
        return false;

    Field* fields = result->Fetch();
    return fields[0].Get<uint64>() == 1;
}

void WarContentController::RetryBossKills()
{
    if (_pendingBossKills.empty())
        return;
    auto const now = std::chrono::steady_clock::now();
    if (now < _nextBossWrite)
        return;
    _nextBossWrite = now + RetryDelay;
    for (auto it = _pendingBossKills.begin(); it != _pendingBossKills.end();)
    {
        if (PersistBossKill(it->CampaignId, it->Origin, it->Entry, it->KilledAt))
        {
            LOG_INFO("module", "AQWarEffort: Named boss {} death durably recorded for campaign {}, war origin {}.",
                it->Entry, it->CampaignId, it->Origin);
            it = _pendingBossKills.erase(it);
            _bossWriteFailureLogged = false;
        }
        else
        {
            if (!_bossWriteFailureLogged)
                LOG_ERROR("module", "AQWarEffort: Could not verify named boss death in character DB. "
                    "Boss stays defeated in this process; retrying persistence. A crash before recovery can lose this kill.");
            _bossWriteFailureLogged = true;
            ++it;
        }
    }
}

WarContentController::BossState WarContentController::GetBossState(WarContentState const& state, uint32 entry)
{
    if (!IsNamedWarBoss(entry) || !state.Active())
        return BossState::Unknown;
    if (_bossCampaignId != state.CampaignId || _bossOrigin != state.Origin)
    {
        _bossCampaignId = state.CampaignId;
        _bossOrigin = state.Origin;
        _bossStates.fill(BossState::Unknown);
        _nextBossLoad = {};
        _bossLoadFailureLogged = false;
    }
    BossState& resultState = _bossStates[entry - BOSS_COLOSSUS_ZORA];
    for (PendingBossKill const& pending : _pendingBossKills)
        if (pending.CampaignId == state.CampaignId && pending.Origin == state.Origin && pending.Entry == entry)
            return resultState = BossState::Defeated;
    if (resultState != BossState::Unknown)
        return resultState;
    auto const now = std::chrono::steady_clock::now();
    if (now < _nextBossLoad)
        return BossState::Unknown; // Fail closed; no unverified boss spawn.
    _nextBossLoad = now + RetryDelay;
    QueryResult result = CharacterDatabase.Query(
        "SELECT COUNT(*) FROM aq_war_effort_boss_kill "
        "WHERE campaign_id = {} AND war_started_at = {} AND boss_id = {}",
        state.CampaignId, state.Origin, entry);
    if (!result)
    {
        if (!_bossLoadFailureLogged)
            LOG_ERROR("module", "AQWarEffort: Cannot read named boss kills from character DB; boss spawning paused.");
        _bossLoadFailureLogged = true;
        return BossState::Unknown;
    }
    _bossLoadFailureLogged = false;
    Field* fields = result->Fetch();
    return resultState = fields[0].Get<uint64>() ? BossState::Defeated : BossState::Alive;
}

void WarContentController::OnOwnedBossDeath(Creature const* creature, WarContentState const& state)
{
    if (!creature || !IsNamedWarBoss(creature->GetEntry()) || !state.Active())
        return;
    std::lock_guard lock(_bossMutex);
    if (!_active)
        return;
    // Only a GUID held by the current module-owned battlefront is eligible.
    // Future fronts register their owned boss slots here, not by area or entry.
    BattlefrontSlot const& slot = _ashi.Slots[std::size(AshiSpawns) - 1];
    if (creature->GetEntry() != BOSS_COLOSSUS_ASHI
        || !IsOwnedWarBossDeath(WarBossKey{ state.CampaignId, state.Origin, creature->GetEntry() },
            WarBossKey{ _campaignId, _origin, BOSS_COLOSSUS_ASHI }, creature->GetGUID(),
            slot.Guid, slot.Spawned, _ashi.Stage == 4))
        return;
    BossState& boss = _bossStates[BOSS_COLOSSUS_ASHI - BOSS_COLOSSUS_ZORA];
    if (boss == BossState::Defeated)
        return;
    boss = BossState::Defeated; // Suppress locally even if the DB is unavailable.
    std::time_t const now = std::time(nullptr);
    _pendingBossKills.push_back({ state.CampaignId, state.Origin, BOSS_COLOSSUS_ASHI,
        now > 0 ? uint64(now) : state.Origin });
    _nextBossWrite = {};
    RetryBossKills();
}

void WarContentController::Cleanup()
{
    // Do not load grids for cleanup, and never remove stock/quest objects by entry.
    Map* map = sMapMgr->FindMap(ContentMap, 0);
    if (_ashi.Stage)
        LOG_INFO("module", "AQWarEffort: Hive'Ashi battlefront cleaned up.");
    CleanupAshi(map);
    if (map)
        if (GameObject* crystal = map->GetGameObject(_crystal))
        {
            crystal->Delete(); // Core queues actual destruction for the next map update.
            crystal->SetPhaseMask(0, true); // Already absent while destruction is pending.
            crystal->EnableCollision(false);
            crystal->setActive(false);
        }
    _crystal.Clear();
    _active = false;
    _failureLogged = false;
    _nextAttempt = {};
}

void WarContentController::CleanupAshi(Map* map)
{
    for (BattlefrontSlot& slot : _ashi.Slots)
    {
        if (map && slot.Spawned)
            if (Creature* creature = map->GetCreature(slot.Guid))
            {
                creature->SetPhaseMask(0, true); // Hide while core removal is queued.
                creature->DespawnOrUnsummon();
            }
        slot = {};
    }
    _ashi = {};
}

void WarContentController::ReconcileAshi(WarContentState const& state, Map* map)
{
    uint8 const stage = BattlefrontStage(state.Elapsed, state.Duration);
    if (_ashi.Stage != stage)
    {
        uint8 const previous = _ashi.Stage;
        CleanupAshi(map);
        _ashi.Stage = stage;
        LOG_INFO("module", "AQWarEffort: Hive'Ashi battlefront {} at stage {}.",
            previous ? "advanced" : "activated", stage);
    }

    auto const now = std::chrono::steady_clock::now();
    if (now < _ashi.NextAttempt)
        return;
    _ashi.NextAttempt = now + RetryDelay;
    for (std::size_t i = 0; i < std::size(AshiSpawns); ++i)
    {
        SpawnDefinition const& definition = AshiSpawns[i];
        BattlefrontSlot& slot = _ashi.Slots[i];
        if (definition.FirstStage > stage || slot.Spawned)
            continue;
        if (definition.Entry == AshiColossus && GetBossState(state, AshiColossus) != BossState::Alive)
            continue;
        if (!sObjectMgr->GetCreatureTemplate(definition.Entry))
        {
            if (!_ashi.FailureLogged)
                LOG_ERROR("module", "AQWarEffort: Hive'Ashi requires stock creature templates 15421, 15758 and 15742.");
            _ashi.FailureLogged = true;
            continue;
        }
        // Explicit GUID ownership; absence after a successful summon means
        // killed or removed, and never triggers a same-stage replacement.
        map->LoadGrid(definition.Location.GetPositionX(), definition.Location.GetPositionY());
        if (TempSummon* creature = map->SummonCreature(definition.Entry, definition.Location))
        {
            slot.Guid = creature->GetGUID();
            slot.Spawned = true;
            creature->setActive(true);
            _ashi.FailureLogged = false;
            if (definition.Entry == AshiColossus)
                LOG_INFO("module", "AQWarEffort: Colossus of Ashi entered the battle.");
        }
        else
        {
            if (!_ashi.FailureLogged)
                LOG_ERROR("module", "AQWarEffort: Could not summon Hive'Ashi battlefront; retrying active slots.");
            _ashi.FailureLogged = true;
        }
    }
}

void WarContentController::Reconcile(WarContentState const& state, bool startup)
{
    std::lock_guard lock(_bossMutex);
    RetryBossKills(); // Also flush deaths after phase cleanup or a new war epoch.
    bool desired = state.Active();
    if (_active && (!desired || state.CampaignId != _campaignId || state.Origin != _origin))
    {
        Cleanup();
        LOG_INFO("module", "AQWarEffort: Temporary war content deactivated for campaign ID {}; phase {}.",
            _campaignId, Manager::PhaseName(state.Phase));
    }
    if (!desired)
    {
        if (startup)
            LOG_INFO("module", "AQWarEffort: Temporary war content absent at startup; "
                "phase {}, managed {}, timing valid {}.",
                Manager::PhaseName(state.Phase), state.Managed, state.TimingValid);
        return;
    }

    _active = true;
    _campaignId = state.CampaignId;
    _origin = state.Origin;
    Map* map = sMapMgr->FindMap(ContentMap, 0);
    if (!map)
        map = sMapMgr->CreateBaseMap(ContentMap);
    if (map)
        ReconcileAshi(state, map);
    if (map && map->GetGameObject(_crystal))
        return; // Exactly one owned formation, regardless of reconciliation frequency.
    _crystal.Clear(); // A grid unload/manual removal cannot leave a dangling pointer.
    auto now = std::chrono::steady_clock::now();
    if (now < _nextAttempt)
        return;
    _nextAttempt = now + RetryDelay; // Only retry pacing, never the campaign clock.

    auto const* info = sObjectMgr->GetGameObjectTemplate(CrystalEntry);
    // Fail closed on incompatible local template overrides; don't patch shared data.
    if (!info || info->type != GAMEOBJECT_TYPE_DOOR || info->displayId != CrystalDisplay
        || info->GetLinkedGameObjectEntry() || info->ScriptId || !info->AIName.empty())
    {
        if (!_failureLogged)
            LOG_ERROR("module", "AQWarEffort: War proof requires stock GO 180810 (door, display 6573, no script/link). "
                "Content inactive; campaign progression unchanged.");
        _failureLogged = true;
        return;
    }
    if (map)
    {
        map->LoadGrid(CrystalPosition.GetPositionX(), CrystalPosition.GetPositionY());
        // No DB spawn, no SaveToDB, no game-event membership, and no independent lifetime.
        if (GameObject* crystal = map->SummonGameObject(CrystalEntry, CrystalPosition, 0, 0, 0, 1, 0))
        {
            _crystal = crystal->GetGUID();
            crystal->SetGameObjectFlag(GO_FLAG_NOT_SELECTABLE);
            crystal->EnableCollision(false); // Visible proof, not another wall/barrier.
            // Keep updating without nearby players; lost objects are reconciled above.
            crystal->setActive(true);
            _failureLogged = false;
            LOG_INFO("module", "AQWarEffort: War proof {} for campaign ID {}: GO 180810 at Scarab Wall; "
                "origin {}, duration {}s, elapsed {}s, remaining {}s, progress {:.3f}.",
                startup ? "restored at startup" : "activated/reconciled", state.CampaignId,
                state.Origin, state.Duration, state.Elapsed, state.Remaining, state.Progress());
            return;
        }
    }
    if (!_failureLogged)
        LOG_ERROR("module", "AQWarEffort: Could not create temporary war proof; "
            "retrying while campaign remains active.");
    _failureLogged = true;
}

class aq_named_war_boss_death : public UnitScript
{
public:
    aq_named_war_boss_death() : UnitScript("aq_named_war_boss_death", true, { UNITHOOK_ON_UNIT_DEATH }) { }

    void OnUnitDeath(Unit* unit, Unit* /*killer*/) override
    {
        if (Creature* creature = unit ? unit->ToCreature() : nullptr)
            WarContentController::Instance().OnOwnedBossDeath(creature, Manager::Instance().GetWarContentState());
    }
};

void RegisterWarContentScripts()
{
    new aq_named_war_boss_death();
}
}
