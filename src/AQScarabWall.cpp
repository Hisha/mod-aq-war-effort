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
    struct WallPart { uint32 Spawn; uint32 Entry; };
    constexpr WallPart WallParts[] =
    {
        { 9100147, 176147 },
        { 9100148, 176148 },
        { 9100146, 176146 }
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
        if (!ControlsWall() || !IsOwnedWall(go))
            return;
        AQCampaignPhase phase = Manager::Instance().GetPhase();
        bool closed = phase == AQ_PHASE_WAR_EFFORT || phase == AQ_PHASE_READY;
        // Keep the stock closed pose. Opening is absence, not an animation/ceremony.
        // Phase mask zero makes the wall absent without playing its opening animation.
        if (go->GetPhaseMask() != (closed ? 1u : 0u))
            go->SetPhaseMask(closed ? 1u : 0u, true);
        if (go->GetGoState() != GO_STATE_READY)
            go->SetGoState(GO_STATE_READY);
        // Visibility alone is not sufficient: explicitly update physical collision too.
        go->EnableCollision(closed);
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
    if (!ControlsWall())
        return;
    Map* map = sMapMgr->CreateBaseMap(WallMap);
    // Load the canonical grid even when nobody has visited Silithus since startup.
    // AddWorld also reconciles lazy reloads, with no retained GameObject pointers.
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
            GetPhase() == AQ_PHASE_WAR_EFFORT || GetPhase() == AQ_PHASE_READY ? "closed" : "absent",
            GetId(), PhaseName(GetPhase()));
}

void RegisterScarabWallScripts()
{
    new aq_scarab_wall_objects();
    new go_aq_war_effort_wall();
}
}
