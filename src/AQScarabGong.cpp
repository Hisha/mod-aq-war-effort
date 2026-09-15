/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "AQWarEffort.h"
#include "AllGameObjectScript.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "GameObjectScript.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "QuestDef.h"
#include "ServerScript.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace AQWarEffort
{
namespace
{
    constexpr uint32 GongSpawn = 9100717;
    constexpr uint32 GongEntry = 180717;
    constexpr char GongScript[] = "go_aq_war_effort_gong";

    bool IsOwnedGong(GameObject* go)
    {
        return go && go->GetMapId() == 1 && go->GetSpawnId() == GongSpawn
            && go->GetEntry() == GongEntry && go->GetScriptId() == sObjectMgr->GetScriptId(GongScript);
    }

    void Deny(Player* player)
    {
        player->PlayerTalkClass->SendCloseGossip();
        ChatHandler(player->GetSession()).SendSysMessage("The Scarab Gong cannot accept a ringing at this time.");
    }

    class aq_scarab_gong_packets : public ServerScript
    {
    public:
        aq_scarab_gong_packets() : ServerScript("aq_scarab_gong_packets", { SERVERHOOK_CAN_PACKET_RECEIVE }) { }

        bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
        {
            if (packet.GetOpcode() != CMSG_QUESTGIVER_CHOOSE_REWARD)
                return true;
            // Read a copy: never advance the stock handler's cursor. The opcode uses an unpacked GUID.
            WorldPacket copy(packet);
            if (copy.size() - copy.rpos() < sizeof(uint64) + 2 * sizeof(uint32))
                return false;
            ObjectGuid guid;
            uint32 questId;
            uint32 reward;
            copy >> guid >> questId >> reward;
            if (questId != QuestBangGong)
                return true;
            Player* player = session ? session->GetPlayer() : nullptr;
            if (!player)
                return false;
            // Also reject legacy/alternate givers of this one quest; they cannot authorize our campaign.
            GameObject* go = player->GetMap()->GetGameObject(guid);
            if (reward >= QUEST_REWARD_CHOICES_COUNT || !Manager::Instance().PrepareGong(player, go))
            {
                Deny(player);
                return false;
            }
            // Eligibility, interaction range, items and rewards remain entirely in QuestHandler/Player.
            return true;
        }
    };

    class aq_scarab_gong_rewards : public AllGameObjectScript
    {
    public:
        aq_scarab_gong_rewards() : AllGameObjectScript("aq_scarab_gong_rewards") { }

        void OnGameObjectAddWorld(GameObject* go) override
        {
            // Stock 180717 carries INTERACT_COND. Clear it only on our dedicated instance.
            if (IsOwnedGong(go))
                go->RemoveGameObjectFlag(GO_FLAG_INTERACT_COND);
        }

        bool CanGameObjectQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 /*opt*/) override
        {
            // Despite its name this hook is AFTER RewardQuest, not a reward veto.
            if (quest->GetQuestId() == QuestBangGong && IsOwnedGong(go))
                Manager::Instance().ObserveGongReward(player, go);
            return false;
        }
    };

    class go_aq_war_effort_gong : public GameObjectScript
    {
    public:
        go_aq_war_effort_gong() : GameObjectScript(GongScript) { }
        bool OnGossipHello(Player* player, GameObject* go) override
        {
            if (!IsOwnedGong(go))
                return false;
            if (!Manager::Instance().GongAvailable())
            {
                Deny(player);
                return true;
            }
            return false;
        }
    };
}

bool Manager::GongAvailable() const
{
    std::lock_guard lock(_mutex);
    return _enabled && _loaded && _gongHealthy && !_gongPending && _campaign.Phase == AQ_PHASE_READY;
}

void Manager::InitializeGong()
{
    std::lock_guard lock(_mutex);
    _gongHealthy = false;
    _gongPending = false;
    _gongObserved = false;
    _wallCeremony = false;
    _wallEvents.Reset();
    // Aggregate returns one row even when empty, distinguishing an absent schema/query failure.
    QueryResult schema = CharacterDatabase.Query("SELECT COUNT(*) FROM aq_war_effort_gong WHERE id = {}", _id);
    if (!schema)
    {
        LOG_ERROR("module", "AQWarEffort: Gong unavailable; apply character base 002_aq_scarab_gong.sql.");
        return;
    }
    _gongHealthy = true;
    if ((*schema)[0].Get<uint64>())
    {
        QueryResult intent = CharacterDatabase.Query(
            "SELECT player_guid, accepted FROM aq_war_effort_gong WHERE id = {}", _id);
        if (!intent)
        {
            _gongHealthy = false;
            return;
        }
        _gongPlayer = (*intent)[0].Get<uint32>();
        _gongPending = !(*intent)[1].Get<bool>();
    }
    // No live observation on startup: recovery can open the wall, never replay presentation.
    UpdateGong(0);
}

bool Manager::PrepareGong(Player* player, GameObject* go)
{
    std::lock_guard lock(_mutex);
    if (!IsOwnedGong(go) || !GongAvailable() || player->GetQuestRewardStatus(QuestBangGong))
        return false;
    // Establish that a later durable reward is new, rather than an old character reward.
    // This is recovery bookkeeping, not a replacement for CanRewardQuest.
    uint32 playerId = player->GetGUID().GetCounter();
    _gongPending = true; // An uncertain write also blocks admin phase overrides until recovery.
    CharacterDatabase.DirectExecute(
        "INSERT INTO aq_war_effort_gong (id, player_guid, ready_started_at, prepared_at, accepted) "
        "SELECT id, {}, phase_started_at, UNIX_TIMESTAMP(), 0 FROM aq_war_effort_campaign "
        "WHERE id = {} AND phase = {} AND phase_started_at = {} "
        "AND NOT EXISTS (SELECT 1 FROM character_queststatus_rewarded WHERE guid = {} AND quest = {}) "
        "ON DUPLICATE KEY UPDATE player_guid = VALUES(player_guid), ready_started_at = VALUES(ready_started_at), "
        "prepared_at = VALUES(prepared_at), accepted = 0",
        playerId, _id, uint32(AQ_PHASE_READY), _campaign.PhaseStartedAt, playerId, QuestBangGong);
    QueryResult check = CharacterDatabase.Query(
        "SELECT player_guid, ready_started_at, accepted FROM aq_war_effort_gong WHERE id = {}", _id);
    if (!check || (*check)[0].Get<uint32>() != playerId
        || (*check)[1].Get<uint64>() != _campaign.PhaseStartedAt || (*check)[2].Get<bool>())
    {
        _gongHealthy = false;
        LOG_ERROR("module", "AQWarEffort: Cannot verify gong intent for campaign {}; reward blocked until restart.", _id);
        return false;
    }
    _gongPlayer = playerId;
    _gongPending = true;
    _gongObserved = false;
    return true;
}

void Manager::ObserveGongReward(Player* player, GameObject* go)
{
    std::lock_guard lock(_mutex);
    if (IsOwnedGong(go) && _gongPending && _gongPlayer == player->GetGUID().GetCounter()
        && player->GetQuestRewardStatus(QuestBangGong))
        _gongObserved = true;
    else
        LOG_DEBUG("module", "AQWarEffort: Ignored duplicate/ineligible 8743 gong callback in phase {}.",
            PhaseName(_campaign.Phase));
    // RewardQuest queues an asynchronous character save. Do not assume it is durable here.
}

void Manager::UpdateGong(uint32 diff)
{
    std::lock_guard lock(_mutex);
    UpdateWallCeremony(diff);
    if (!_gongPending || !_gongHealthy || !_loaded || !_enabled)
        return;
    QueryResult result = CharacterDatabase.Query(
        "SELECT g.ready_started_at, EXISTS (SELECT 1 FROM character_queststatus_rewarded r "
        "WHERE r.guid = g.player_guid AND r.quest = {}) FROM aq_war_effort_gong g WHERE g.id = {}",
        QuestBangGong, _id);
    if (!result)
    {
        _gongHealthy = false;
        LOG_ERROR("module", "AQWarEffort: Gong recovery read failed; restart after repairing the database.");
        return;
    }
    if (_campaign.Phase != AQ_PHASE_READY || (*result)[0].Get<uint64>() != _campaign.PhaseStartedAt)
    {
        _gongHealthy = false;
        LOG_ERROR("module", "AQWarEffort: Pending gong belongs to another campaign phase; administrator review required.");
        return;
    }
    if (!(*result)[1].Get<bool>())
    {
        if (_gongObserved)
            return; // Wait for the core's asynchronous reward save. Never discard successful live evidence.
        // World::OnWorldUpdate runs AFTER sessions and completion of map workers. No handler is in flight.
        // At startup, an absent rewarded row means the interrupted attempt did not durably succeed.
        CharacterDatabase.DirectExecute("DELETE FROM aq_war_effort_gong WHERE id = {} AND accepted = 0", _id);
        QueryResult cleared = CharacterDatabase.Query("SELECT COUNT(*) FROM aq_war_effort_gong WHERE id = {}", _id);
        if (!cleared || (*cleared)[0].Get<uint64>())
        {
            _gongHealthy = false;
            return;
        }
        _gongPending = false;
        return;
    }
    Campaign accepted = _campaign;
    EnterPhase(accepted, AQ_PHASE_TEN_HOUR_WAR);
    accepted.GongRungAt = accepted.PhaseStartedAt;
    if (!Persist(accepted, true))
    {
        _gongHealthy = false;
        LOG_ERROR("module", "AQWarEffort: Gong acceptance could not be verified; durable intent retained for restart.");
        return;
    }
    _gongPending = false;
    if (_gongObserved)
        BeginWallCeremony();
    else
        SyncWall();
    LOG_INFO("module", "AQWarEffort: Gong accepted for campaign {}, player {}, presentation {}.",
        _id, _gongPlayer, _gongObserved ? "live" : "skipped on recovery");
    _gongObserved = false;
}

void RegisterScarabGongScripts()
{
    new aq_scarab_gong_packets();
    new aq_scarab_gong_rewards();
    new go_aq_war_effort_gong();
}
}
