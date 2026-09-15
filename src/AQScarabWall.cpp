/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * Placement follows AzerothCore zone_silithus.cpp, quest 8519.
 */
#include "AQWarEffort.h"
#include "AllGameObjectScript.h"
#include "GameObject.h"
#include "GameObjectScript.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectMgr.h"

namespace AQWarEffort
{
namespace
{
    constexpr uint32 WallMap = 1;
    constexpr float WallX = -8130.0f;
    constexpr float WallY = 1525.0f;
    constexpr char WallScript[] = "go_aq_war_effort_wall";

    // Reserved, collision-checked by 002_aq_scarab_wall.sql; never match by entry alone.
    // Provisional client timings: animate each piece for 4 seconds, then hide it.
    // Order and sounds follow the historical opening. See docs/SCARAB_GONG.md.
    constexpr uint32 PartAnimationMs = 4000;
    constexpr uint32 CeremonyDurationMs = 3 * PartAnimationMs;
    struct WallPart { uint32 Spawn; uint32 Entry; uint32 Sound; uint32 StartsAt; };
    constexpr WallPart WallParts[] =
    {
        { 9100147, 176147, 7114, 0 },
        { 9100148, 176148, 7116, PartAnimationMs },
        { 9100146, 176146, 7115, 2 * PartAnimationMs }
    };

    bool ControlsWall()
    {
        auto& manager = Manager::Instance();
        return manager.IsEnabled() && manager.IsAvailable() && manager.GetPhase() != AQ_PHASE_DISABLED;
    }

    bool IsOwnedWall(GameObject* go)
    {
        for (WallPart const& part : WallParts)
            if (go->GetSpawnId() == part.Spawn && go->GetEntry() == part.Entry)
                return go->GetMapId() == WallMap && go->GetScriptId() == sObjectMgr->GetScriptId(WallScript);
        return false;
    }

    void ReconcilePart(GameObject* go)
    {
        auto& manager = Manager::Instance();
        std::lock_guard lock(manager.Mutex());
        if (!ControlsWall() || !IsOwnedWall(go))
            return;
        AQCampaignPhase phase = manager.GetPhase();
        bool closed = phase == AQ_PHASE_WAR_EFFORT || phase == AQ_PHASE_READY;
        bool visible = closed;
        GOState pose = GO_STATE_READY;
        if (manager.WallCeremonyActive() && phase == AQ_PHASE_TEN_HOUR_WAR)
        {
            for (WallPart const& part : WallParts)
                if (go->GetSpawnId() == part.Spawn)
                {
                    uint32 elapsed = manager.WallCeremonyElapsed();
                    closed = elapsed < part.StartsAt;
                    visible = elapsed < part.StartsAt + PartAnimationMs;
                    pose = closed ? GO_STATE_READY : GO_STATE_ACTIVE;
                }
        }
        if (go->GetPhaseMask() != (visible ? 1u : 0u))
            go->SetPhaseMask(visible ? 1u : 0u, true);
        if (go->GetGoState() != pose)
            go->SetGoState(pose);
        go->EnableCollision(closed);
    }

    void PlayStageSound(uint32 stage)
    {
        Map* map = sMapMgr->CreateBaseMap(WallMap);
        if (!map)
            return;
        for (auto const& [spawnId, go] : map->GetGameObjectBySpawnIdStore())
            if (spawnId == WallParts[stage].Spawn && IsOwnedWall(go))
            {
                go->PlayDistanceSound(WallParts[stage].Sound);
                break;
            }
    }

    class aq_scarab_wall_objects : public AllGameObjectScript
    {
    public:
        aq_scarab_wall_objects() : AllGameObjectScript("aq_scarab_wall_objects") { }
        void OnGameObjectAddWorld(GameObject* go) override { ReconcilePart(go); }
        void OnGameObjectUpdate(GameObject* go, uint32 /*diff*/) override { ReconcilePart(go); }
    };

    class go_aq_war_effort_wall : public GameObjectScript
    {
    public:
        go_aq_war_effort_wall() : GameObjectScript(WallScript) { }
        bool OnGossipHello(Player* /*player*/, GameObject* go) override
        {
            // Doors/buttons must not be opened by ordinary right-click while controlled.
            // DISABLED leaves the stock behavior alone.
            return ControlsWall() && IsOwnedWall(go);
        }
    };
}

void Manager::SyncWall()
{
    std::lock_guard lock(_mutex);
    if (!ControlsWall())
        return;
    Map* map = sMapMgr->CreateBaseMap(WallMap);
    // Load the canonical grid even when nobody has visited Silithus since startup.
    // AddWorld also reconciles lazy reloads, with no retained GameObject pointers.
    if (!map)
        return;
    map->LoadGrid(WallX, WallY);
    uint32 found = 0;
    for (auto const& [spawnId, go] : map->GetGameObjectBySpawnIdStore())
    {
        if (IsOwnedWall(go))
        {
            ReconcilePart(go);
            ++found;
        }
    }
    if (found != 3)
        LOG_ERROR("module", "AQWarEffort: Scarab Wall found {} of 3 owned objects. Apply 002_aq_scarab_wall.sql and restart.", found);
    else
        LOG_INFO("module", "AQWarEffort: Scarab Wall reconciled {} for campaign ID {}, phase {}.",
            _wallCeremony ? "opening ceremony" :
                (GetPhase() == AQ_PHASE_WAR_EFFORT || GetPhase() == AQ_PHASE_READY ? "closed" : "absent"),
            GetId(), PhaseName(GetPhase()));
}

void Manager::BeginWallCeremony()
{
    std::lock_guard lock(_mutex);
    _wallEvents.Reset();
    _wallCeremony = false;
    if (!ControlsWall() || _campaign.Phase != AQ_PHASE_TEN_HOUR_WAR)
        return;
    // Missing pieces must not leave a partial ceremony.
    Map* map = sMapMgr->CreateBaseMap(WallMap);
    if (map)
        map->LoadGrid(WallX, WallY);
    uint32 found = 0;
    if (map)
        for (auto const& [spawnId, go] : map->GetGameObjectBySpawnIdStore())
            if (IsOwnedWall(go))
                ++found;
    if (found != 3)
    {
        SyncWall();
        LOG_WARN("module", "AQWarEffort: Opening presentation skipped: {} of 3 wall parts available; campaign remains TEN_HOUR_WAR.", found);
        return;
    }
    _wallCeremony = true;
    _wallCeremonyElapsed = 0;
    SyncWall();
    PlayStageSound(0);
    // Event IDs 1/2 start runes/gate; 3 settles the permanent open state.
    for (uint32 stage = 1; stage <= 3; ++stage)
        _wallEvents.ScheduleEvent(stage, Milliseconds(stage * PartAnimationMs));
}

void Manager::UpdateWallCeremony(uint32 diff)
{
    std::lock_guard lock(_mutex);
    if (!_wallCeremony)
        return;
    _wallEvents.Update(diff);
    while (uint32 stage = _wallEvents.ExecuteEvent())
    {
        _wallCeremonyElapsed = stage * PartAnimationMs;
        if (_wallCeremonyElapsed >= CeremonyDurationMs)
            _wallCeremony = false;
        Map* map = sMapMgr->CreateBaseMap(WallMap);
        if (!map)
        {
            _wallCeremony = false;
            _wallEvents.Reset();
            return; // AddWorld will apply the persisted final state on later grid load.
        }
        for (auto const& [spawnId, go] : map->GetGameObjectBySpawnIdStore())
            ReconcilePart(go);
        if (_wallCeremony)
            PlayStageSound(stage);
    }
}

void RegisterScarabWallScripts()
{
    new aq_scarab_wall_objects();
    new go_aq_war_effort_wall();
}
}
