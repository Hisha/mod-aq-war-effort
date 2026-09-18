/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * Crystal asset referenced by AzerothCore mod-war-effort warevent.sql.
 * Proof placement uses an AzerothCore zone_silithus.cpp stock battle-scene position.
 */
#include "AQWarContent.h"
#include "AQBattlefrontStage.h"
#include "Creature.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectMgr.h"
#include "TemporarySummon.h"
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
    constexpr uint32 AshiColossus = 15742;
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
}
