/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * Resource mappings and quartermaster text adapted from AzerothCore mod-war-effort.
 */
#include "AQWarEffort.h"
#include "Chat.h"
#include "Config.h"
#include "CommandScript.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "GameEventMgr.h"
#include "GameEventScript.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerScript.h"
#include "QuestDef.h"
#include "ScriptedGossip.h"
#include "WorldScript.h"
#include <charconv>
#include <ctime>
#include <limits>
#include <sstream>

namespace AQWarEffort
{
namespace
{
    constexpr uint16 CollectionEvent = 22;
    constexpr uint32 MaterialDataVersion = 2;

    // Audited against stock quest_template and validated against loaded quests at startup.
    Material const Materials[FactionCount][MaterialCount] =
    {
        {
            { "Linen Bandages", 8517, 8518, MATERIAL_CAT_BANDAGES, 1251, 20,
                "AQWarEffort.Goal.Alliance.Bandages.01" },
            { "Silk Bandages", 8520, 8521, MATERIAL_CAT_BANDAGES, 6450, 20,
                "AQWarEffort.Goal.Alliance.Bandages.02" },
            { "Runecloth Bandages", 8522, 8523, MATERIAL_CAT_BANDAGES, 14529, 20,
                "AQWarEffort.Goal.Alliance.Bandages.03" },
            { "Rainbow Fin Albacore", 8524, 8525, MATERIAL_CAT_FOOD, 5095, 20,
                "AQWarEffort.Goal.Alliance.Food.01" },
            { "Roast Raptor", 8526, 8527, MATERIAL_CAT_FOOD, 12210, 20,
                "AQWarEffort.Goal.Alliance.Food.02" },
            { "Spotted Yellowtail", 8528, 8529, MATERIAL_CAT_FOOD, 6887, 20,
                "AQWarEffort.Goal.Alliance.Food.03" },
            { "Stranglekelp", 8503, 8504, MATERIAL_CAT_HERBS, 3820, 20,
                "AQWarEffort.Goal.Alliance.Herbs.01" },
            { "Arthas' Tears", 8509, 8510, MATERIAL_CAT_HERBS, 8836, 20,
                "AQWarEffort.Goal.Alliance.Herbs.02" },
            { "Purple Lotus", 8505, 8506, MATERIAL_CAT_HERBS, 8831, 20,
                "AQWarEffort.Goal.Alliance.Herbs.03" },
            { "Iron Bars", 8494, 8495, MATERIAL_CAT_METAL, 3575, 20,
                "AQWarEffort.Goal.Alliance.Metal.01" },
            { "Thorium Bars", 8499, 8500, MATERIAL_CAT_METAL, 12359, 20,
                "AQWarEffort.Goal.Alliance.Metal.02" },
            { "Copper Bars", 8492, 8493, MATERIAL_CAT_METAL, 2840, 20,
                "AQWarEffort.Goal.Alliance.Metal.03" },
            { "Light Leather", 8511, 8512, MATERIAL_CAT_LEATHER, 2318, 10,
                "AQWarEffort.Goal.Alliance.Leather.01" },
            { "Medium Leather", 8513, 8514, MATERIAL_CAT_LEATHER, 2319, 10,
                "AQWarEffort.Goal.Alliance.Leather.02" },
            { "Thick Leather", 8515, 8516, MATERIAL_CAT_LEATHER, 4304, 10,
                "AQWarEffort.Goal.Alliance.Leather.03" },
        },
        {
            { "Wool Bandages", 8604, 8605, MATERIAL_CAT_BANDAGES, 3530, 20,
                "AQWarEffort.Goal.Horde.Bandages.01" },
            { "Mageweave Bandages", 8607, 8608, MATERIAL_CAT_BANDAGES, 8544, 20,
                "AQWarEffort.Goal.Horde.Bandages.02" },
            { "Runecloth Bandages", 8609, 8610, MATERIAL_CAT_BANDAGES, 14529, 20,
                "AQWarEffort.Goal.Horde.Bandages.03" },
            { "Lean Wolf Steaks", 8611, 8612, MATERIAL_CAT_FOOD, 12209, 20,
                "AQWarEffort.Goal.Horde.Food.01" },
            { "Baked Salmon", 8615, 8616, MATERIAL_CAT_FOOD, 13935, 20,
                "AQWarEffort.Goal.Horde.Food.02" },
            { "Spotted Yellowtail", 8613, 8614, MATERIAL_CAT_FOOD, 6887, 20,
                "AQWarEffort.Goal.Horde.Food.03" },
            { "Peacebloom", 8549, 8550, MATERIAL_CAT_HERBS, 2447, 20,
                "AQWarEffort.Goal.Horde.Herbs.01" },
            { "Firebloom", 8580, 8581, MATERIAL_CAT_HERBS, 4625, 20,
                "AQWarEffort.Goal.Horde.Herbs.02" },
            { "Purple Lotus", 8582, 8583, MATERIAL_CAT_HERBS, 8831, 20,
                "AQWarEffort.Goal.Horde.Herbs.03" },
            { "Tin Bars", 8542, 8543, MATERIAL_CAT_METAL, 3576, 20,
                "AQWarEffort.Goal.Horde.Metal.01" },
            { "Mithril Bars", 8545, 8546, MATERIAL_CAT_METAL, 3860, 20,
                "AQWarEffort.Goal.Horde.Metal.02" },
            { "Copper Bars", 8532, 8533, MATERIAL_CAT_METAL, 2840, 20,
                "AQWarEffort.Goal.Horde.Metal.03" },
            { "Heavy Leather", 8588, 8589, MATERIAL_CAT_LEATHER, 4234, 10,
                "AQWarEffort.Goal.Horde.Leather.01" },
            { "Rugged Leather", 8600, 8601, MATERIAL_CAT_LEATHER, 8170, 10,
                "AQWarEffort.Goal.Horde.Leather.02" },
            { "Thick Leather", 8590, 8591, MATERIAL_CAT_LEATHER, 4304, 10,
                "AQWarEffort.Goal.Horde.Leather.03" },
        },
    };
}

Manager& Manager::Instance()
{
    static Manager instance;
    return instance;
}

char const* Manager::PhaseName(AQCampaignPhase phase)
{
    switch (phase)
    {
        case AQ_PHASE_DISABLED: return "DISABLED";
        case AQ_PHASE_WAR_EFFORT: return "WAR_EFFORT";
        case AQ_PHASE_READY: return "READY";
        case AQ_PHASE_TEN_HOUR_WAR: return "TEN_HOUR_WAR";
        case AQ_PHASE_OPEN: return "OPEN";
        default: return "UNKNOWN";
    }
}

Material const& Manager::GetMaterial(uint8 faction, uint8 material)
{
    return Materials[faction][material];
}

bool Manager::ValidateQuest(Material const& material, Quest const* quest)
{
    uint32 quantity = 0;
    bool valid = quest != nullptr;
    if (quest)
    {
        for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            if (!quest->RequiredItemId[i] && !quest->RequiredItemCount[i])
                continue;
            if (quest->RequiredItemId[i] != material.ItemId)
                valid = false;
            quantity += quest->RequiredItemCount[i];
        }
    }
    if (!valid || quantity != material.TurnInQuantity)
    {
        LOG_ERROR("module", "AQWarEffort: Invalid quest data for {} (quest {}): expected item {} x{}, found quantity {}. Check both quests {}/{}.",
            material.Name, quest ? quest->GetQuestId() : 0, material.ItemId, material.TurnInQuantity,
            quantity, material.Quest, material.RepeatQuest);
        return false;
    }
    return true;
}

bool Manager::ValidateConfiguration()
{
    bool valid = true;
    for (std::string const& key : sConfigMgr->GetKeysByString("AQWarEffort.Goal."))
    {
        bool known = false;
        for (auto const& faction : Materials)
            for (Material const& material : faction)
                if (key == material.GoalKey)
                    known = true;
        if (!known)
        {
            LOG_ERROR("module", "AQWarEffort: Unknown goal key {}. Use the .conf.dist category keys (.01/.02/.03).", key);
            valid = false;
        }
    }
    for (uint8 faction = 0; faction < FactionCount; ++faction)
    {
        for (uint8 i = 0; i < MaterialCount; ++i)
        {
            Material const& material = Materials[faction][i];
            std::string configured = sConfigMgr->GetOption<std::string>(material.GoalKey,
                std::to_string((i % 3 + 1) * material.TurnInQuantity));
            uint64 goal = 0;
            auto parsed = std::from_chars(configured.data(), configured.data() + configured.size(), goal);
            _goals[faction][i] = goal;
            if (parsed.ec != std::errc() || parsed.ptr != configured.data() + configured.size()
                || goal < material.TurnInQuantity || goal % material.TurnInQuantity != 0)
            {
                LOG_ERROR("module", "AQWarEffort: Invalid {} {} goal {} ({}): must be at least {} and a multiple of {}.",
                    faction == TEAM_ALLIANCE ? "Alliance" : "Horde", material.Name, configured,
                    material.GoalKey, material.TurnInQuantity, material.TurnInQuantity);
                valid = false;
            }
            // Evaluate every entry so administrators see all problems in one startup.
            if (!ValidateQuest(material, sObjectMgr->GetQuestTemplate(material.Quest)))
                valid = false;
            if (!ValidateQuest(material, sObjectMgr->GetQuestTemplate(material.RepeatQuest)))
                valid = false;
        }
    }
    return valid;
}

void Manager::SyncCollectionEvent()
{
    if (!_initialized || !_enabled || _syncingEvent)
        return;
    auto const& events = sGameEventMgr->GetEventMap();
    if (CollectionEvent >= events.size() || !events[CollectionEvent].isValid())
    {
        LOG_ERROR("module", "AQWarEffort: Required collection game event 22 is missing or invalid.");
        return;
    }
    // Invalid configuration/data must not expose collectors while progress cannot be saved.
    bool desired = _loaded && _campaign.Phase == AQ_PHASE_WAR_EFFORT;
    if (sGameEventMgr->IsActiveEvent(CollectionEvent) == desired)
        return;
    _syncingEvent = true;
    // overwrite=false preserves configured dates, including in-memory schedule dates.
    if (desired)
        sGameEventMgr->StartEvent(CollectionEvent, false);
    else
        sGameEventMgr->StopEvent(CollectionEvent, false);
    _syncingEvent = false;
    if (sGameEventMgr->IsActiveEvent(CollectionEvent) == desired)
        LOG_INFO("module", "AQWarEffort: Game event 22 {} for campaign ID {}, phase {}{}.",
            desired ? "started" : "stopped", _id, PhaseName(_campaign.Phase), _loaded ? "" : " (tracking unavailable)");
    else
        LOG_ERROR("module", "AQWarEffort: Could not synchronize game event 22 for campaign ID {}; check event disable settings.", _id);
}

bool Manager::IsComplete(Campaign const& campaign, uint8 faction) const
{
    for (uint8 i = 0; i < MaterialCount; ++i)
    {
        if (campaign.Contributions[faction][i] < _goals[faction][i])
            return false;
    }
    return true;
}

bool Manager::IsComplete(uint8 faction) const
{
    return _loaded && faction < FactionCount && IsComplete(_campaign, faction);
}

bool Manager::IsComplete() const
{
    return IsComplete(TEAM_ALLIANCE) && IsComplete(TEAM_HORDE);
}

void Manager::Initialize()
{
    std::lock_guard lock(_mutex);
    _loaded = false;
    _enabled = sConfigMgr->GetOption<bool>("AQWarEffort.Enable", false);
    _id = sConfigMgr->GetOption<uint32>("AQWarEffort.Id", 1);
    _initialized = true;
    if (!ValidateConfiguration())
    {
        LOG_ERROR("module", "AQWarEffort: Invalid goals or quest data; no campaign data changed. Tracking disabled until restart.");
        SyncCollectionEvent();
        return;
    }
    QueryResult version = CharacterDatabase.Query("SELECT material_data_version FROM aq_war_effort_schema WHERE id = 1");
    if (!version || (*version)[0].Get<uint32>() != MaterialDataVersion)
    {
        LOG_ERROR("module", "AQWarEffort: Material data version 2 required. Stop worldserver and apply the character update SQL.");
        SyncCollectionEvent();
        return;
    }

    // Initialize only absent rows; disabled configuration does not rewrite persistent state.
    CharacterDatabase.DirectExecute("INSERT INTO aq_war_effort (id, faction) VALUES ({}, 0), ({}, 1) "
        "ON DUPLICATE KEY UPDATE id = id", _id, _id);

    Campaign initial;
    QueryResult materials = CharacterDatabase.Query(
        "SELECT faction, bandages01, bandages02, bandages03, food01, food02, food03, "
        "herbs01, herbs02, herbs03, metals01, metals02, metals03, leather01, leather02, leather03 "
        "FROM aq_war_effort WHERE id = {} AND faction IN (0, 1) ORDER BY faction", _id);
    if (!materials || materials->GetRowCount() != FactionCount)
    {
        LOG_ERROR("module", "AQWarEffort: Cannot load supplies for ID {}; apply the character schema and restart.", _id);
        SyncCollectionEvent();
        return;
    }
    do
    {
        uint8 faction = (*materials)[0].Get<uint8>();
        for (uint8 i = 0; i < MaterialCount; ++i)
            initial.Contributions[faction][i] = (*materials)[i + 1].Get<uint64>();
    } while (materials->NextRow());

    EnterPhase(initial, IsComplete(initial, TEAM_ALLIANCE) && IsComplete(initial, TEAM_HORDE)
        ? AQ_PHASE_READY : AQ_PHASE_WAR_EFFORT);
    CharacterDatabase.DirectExecute("INSERT INTO aq_war_effort_campaign (id, phase, phase_started_at) "
        "VALUES ({}, {}, {}) ON DUPLICATE KEY UPDATE id = id", _id, uint32(initial.Phase), initial.PhaseStartedAt);
    if (!ReadCampaign(_campaign))
    {
        LOG_ERROR("module", "AQWarEffort: Cannot load campaign ID {}. Check schema, phase and counters; tracking disabled.", _id);
        SyncCollectionEvent();
        return;
    }
    _loaded = true;
    InitializeGong();
    LOG_INFO("module", "AQWarEffort: Loaded campaign ID {}, phase {}, tracking {}", _id,
        PhaseName(_campaign.Phase), _enabled ? "enabled" : "disabled");
    SyncCollectionEvent();
    SyncWall();
}

bool Manager::ReadCampaign(Campaign& campaign) const
{
    // One query reads state and both factions from the same committed snapshot.
    QueryResult result = CharacterDatabase.Query(
        "SELECT c.phase, c.phase_started_at, c.gong_rung_at, c.opened_at, m.faction, "
        "m.bandages01, m.bandages02, m.bandages03, m.food01, m.food02, m.food03, "
        "m.herbs01, m.herbs02, m.herbs03, m.metals01, m.metals02, m.metals03, "
        "m.leather01, m.leather02, m.leather03 FROM aq_war_effort_campaign c "
        "JOIN aq_war_effort m ON m.id = c.id WHERE c.id = {} AND m.faction IN (0, 1) ORDER BY m.faction", _id);
    if (!result || result->GetRowCount() != FactionCount || (*result)[0].Get<uint8>() > AQ_PHASE_OPEN)
        return false;
    campaign.Phase = AQCampaignPhase((*result)[0].Get<uint8>());
    campaign.PhaseStartedAt = (*result)[1].Get<uint64>();
    campaign.GongRungAt = (*result)[2].Get<uint64>();
    campaign.OpenedAt = (*result)[3].Get<uint64>();
    do
    {
        uint8 faction = (*result)[4].Get<uint8>();
        for (uint8 i = 0; i < MaterialCount; ++i)
        {
            uint64 count = (*result)[i + 5].Get<uint64>();
            campaign.Contributions[faction][i] = count;
        }
    } while (result->NextRow());
    return true;
}

void Manager::EnterPhase(Campaign& campaign, AQCampaignPhase phase)
{
    campaign.Phase = phase;
    campaign.PhaseStartedAt = uint64(std::time(nullptr));
    if (phase == AQ_PHASE_TEN_HOUR_WAR)
        campaign.GongRungAt = campaign.PhaseStartedAt;
    if (phase == AQ_PHASE_OPEN)
        campaign.OpenedAt = campaign.PhaseStartedAt;
}

bool Manager::Persist(Campaign const& next)
{
    std::lock_guard lock(_mutex);
    if (!_loaded)
        return false;
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    for (uint8 faction = 0; faction < FactionCount; ++faction)
    {
        auto const& c = next.Contributions[faction];
        transaction->Append("UPDATE aq_war_effort SET bandages01 = {}, bandages02 = {}, bandages03 = {}, "
            "food01 = {}, food02 = {}, food03 = {}, herbs01 = {}, herbs02 = {}, herbs03 = {}, "
            "metals01 = {}, metals02 = {}, metals03 = {}, leather01 = {}, leather02 = {}, leather03 = {} "
            "WHERE id = {} AND faction = {}", c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8],
            c[9], c[10], c[11], c[12], c[13], c[14], _id, uint32(faction));
    }
    transaction->Append("UPDATE aq_war_effort_campaign SET phase = {}, phase_started_at = {}, "
        "gong_rung_at = {}, opened_at = {} WHERE id = {}", uint32(next.Phase), next.PhaseStartedAt,
        next.GongRungAt, next.OpenedAt, _id);
    CharacterDatabase.DirectCommitTransaction(transaction);

    // The core's synchronous transaction API returns void. Verify before publishing state.
    Campaign persisted;
    if (!ReadCampaign(persisted) || !(persisted == next))
    {
        _loaded = false;
        LOG_ERROR("module", "AQWarEffort: Campaign ID {} write could not be verified. Tracking stopped; inspect DB and restart.", _id);
        SyncCollectionEvent();
        return false;
    }
    bool phaseChanged = _campaign.Phase != next.Phase;
    if (phaseChanged)
        LOG_INFO("module", "AQWarEffort: Campaign ID {} transitioned {} -> {}", _id,
            PhaseName(_campaign.Phase), PhaseName(next.Phase));
    _campaign = next;
    SyncCollectionEvent();
    if (phaseChanged)
        SyncWall();
    return true;
}

bool Manager::SetPhase(AQCampaignPhase phase)
{
    std::lock_guard lock(_mutex);
    if (_gongPending || !_loaded || phase > AQ_PHASE_OPEN)
        return false;
    _wallCeremony = false;
    if (phase == _campaign.Phase)
    {
        SyncCollectionEvent();
        SyncWall();
        return true;
    }
    Campaign next = _campaign;
    EnterPhase(next, phase);
    return Persist(next);
}

void Manager::OnQuestReward(Player* player, Quest const* quest)
{
    std::lock_guard lock(_mutex);
    uint32 questId = quest->GetQuestId();
    // Gong acceptance belongs to the giver-specific post-reward observer.
    if (questId == QuestBangGong)
        return;

    // Classify by the rewarded quest, not the player's current faction (e.g. cross-faction play).
    for (uint8 faction = 0; faction < FactionCount; ++faction)
    {
        for (uint8 i = 0; i < MaterialCount; ++i)
        {
            Material const& material = Materials[faction][i];
            if (questId != material.Quest && questId != material.RepeatQuest)
                continue;
            char const* reason = !_enabled ? "AQWarEffort.Enable is disabled"
                : !_loaded ? "campaign unavailable; check startup/database errors"
                : _campaign.Phase != AQ_PHASE_WAR_EFFORT ? "campaign is not in WAR_EFFORT" : nullptr;
            if (reason)
            {
                LOG_WARN("module", "AQWarEffort: Quest {} ({}) not counted for campaign ID {}: {}", questId, material.Name, _id, reason);
                ChatHandler(player->GetSession()).PSendSysMessage("AQ War Effort contribution not counted: {}.", reason);
                return;
            }
            if (!ValidateQuest(material, quest))
            {
                _loaded = false;
                SyncCollectionEvent();
                ChatHandler(player->GetSession()).SendSysMessage("AQ War Effort quest data changed. Tracking stopped; contact an administrator.");
                return;
            }
            Campaign next = _campaign;
            uint64& total = next.Contributions[faction][i];
            if (total > std::numeric_limits<uint64>::max() - material.TurnInQuantity)
            {
                LOG_ERROR("module", "AQWarEffort: Quantity overflow for campaign ID {}, material {}.", _id, material.Name);
                ChatHandler(player->GetSession()).SendSysMessage("AQ War Effort counter overflow; contact an administrator.");
                return;
            }
            total += material.TurnInQuantity;
            bool ready = IsComplete(next, TEAM_ALLIANCE) && IsComplete(next, TEAM_HORDE);
            if (ready)
                EnterPhase(next, AQ_PHASE_READY);
            if (!Persist(next))
            {
                ChatHandler(player->GetSession()).SendSysMessage("AQ War Effort could not save this contribution. Please contact an administrator.");
                return;
            }
            LOG_DEBUG("module", "AQWarEffort: Rewarded quest {} added {} {} to campaign ID {}, total {}.",
                questId, material.TurnInQuantity, material.Name, _id, total);
            if (ready)
                ChatHandler(player->GetSession()).SendSysMessage("All the required War Effort resources have been gathered. The expedition presses on to Silithus!");
            return;
        }
    }
}

std::string Manager::Scores(uint8 faction, uint8 category) const
{
    if (!_loaded || faction >= FactionCount)
        return "Campaign data unavailable. Check server logs.";
    std::ostringstream text;
    for (uint8 i = 0; i < MaterialCount; ++i)
    {
        Material const& material = Materials[faction][i];
        if (category != MATERIAL_CAT_COUNT && category != material.Category)
            continue;
        text << material.Name << ": " << _campaign.Contributions[faction][i]
            << " / " << _goals[faction][i] << "\n";
    }
    return text.str();
}
}

namespace
{
using namespace AQWarEffort;
using namespace Acore::ChatCommands;

class aq_war_effort_world : public WorldScript
{
public:
    aq_war_effort_world() : WorldScript("aq_war_effort_world", {
        WORLDHOOK_ON_STARTUP, WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_UPDATE }) { }

    void OnStartup() override
    {
        Manager::Instance().Initialize();
    }

    void OnUpdate(uint32 diff) override { Manager::Instance().UpdateGong(diff); }

    void OnAfterConfigLoad(bool reload) override
    {
        if (reload)
            LOG_INFO("module", "AQWarEffort: Settings remain unchanged until worldserver restart.");
    }
};

// Correct external/scheduled changes too, without changing the event's dates.
class aq_war_effort_events : public GameEventScript
{
public:
    aq_war_effort_events() : GameEventScript("aq_war_effort_events", {
        GAMEEVENTHOOK_ON_START, GAMEEVENTHOOK_ON_STOP }) { }

    void OnStart(uint16 eventId) override
    {
        if (eventId == CollectionEvent)
            Manager::Instance().SyncCollectionEvent();
    }

    void OnStop(uint16 eventId) override
    {
        if (eventId == CollectionEvent)
            Manager::Instance().SyncCollectionEvent();
    }
};

class aq_war_effort_player : public PlayerScript
{
public:
    aq_war_effort_player() : PlayerScript("aq_war_effort_player", { PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST }) { }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        // Verified in Player::RewardQuest: this hook runs after the reward, not objective completion.
        Manager::Instance().OnQuestReward(player, quest);
    }
};

class npc_aq_war_effort_quartermaster : public CreatureScript
{
public:
    npc_aq_war_effort_quartermaster() : CreatureScript("npc_aq_war_effort_quartermaster") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        auto& manager = Manager::Instance();
        if (!manager.IsEnabled())
            return false;
        ClearGossipMenuFor(player);
        if (!manager.IsAvailable())
        {
            ChatHandler(player->GetSession()).SendSysMessage("Campaign data unavailable. Please contact an administrator.");
            CloseGossipMenuFor(player);
            return true;
        }
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());
        char const* categories[] = { "bandages", "cooked goods", "herbs", "metal bars", "leather skins" };
        for (uint8 i = 0; i < MATERIAL_CAT_COUNT; ++i)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, std::string("How many ") + categories[i]
                + " have we collected so far?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + i);
        SendGossipMenuFor(player, creature->GetEntry() == QuartermasterHorde ? 8092 : 8082, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        auto& manager = Manager::Instance();
        if (!manager.IsEnabled() || sender != GOSSIP_SENDER_MAIN || action < GOSSIP_ACTION_INFO_DEF
            || action >= GOSSIP_ACTION_INFO_DEF + uint32(MATERIAL_CAT_COUNT))
            return false;
        uint8 faction = creature->GetEntry() == QuartermasterHorde ? TEAM_HORDE : TEAM_ALLIANCE;
        std::string report = faction == TEAM_HORDE
            ? "I just received word on that.\n" : "Good question, $C. Last I was informed:\n";
        report += manager.Scores(faction, uint8(action - GOSSIP_ACTION_INFO_DEF));
        creature->Whisper(report, LANG_UNIVERSAL, player);
        ClearGossipMenuFor(player);
        CloseGossipMenuFor(player);
        return true;
    }
};

class aq_war_effort_commands : public CommandScript
{
public:
    aq_war_effort_commands() : CommandScript("aq_war_effort_commands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commands =
        {
            { "status", Status, SEC_MODERATOR, Console::Yes },
            { "scores", Scores, SEC_MODERATOR, Console::Yes },
            { "phase", Phase, SEC_ADMINISTRATOR, Console::Yes }
        };
        static ChatCommandTable root = { { "aqwareffort", commands } };
        return root;
    }

    static bool Status(ChatHandler* handler)
    {
        auto& manager = Manager::Instance();
        handler->SendSysMessage("Ahn'Qiraj War Effort");
        handler->PSendSysMessage("Campaign ID: {}", manager.GetId());
        handler->PSendSysMessage("Module: {}", manager.IsEnabled() ? "enabled" : "disabled");
        handler->PSendSysMessage("Collection event 22: {}", sGameEventMgr->IsActiveEvent(CollectionEvent) ? "active" : "inactive");
        if (!manager.IsAvailable())
        {
            handler->SendSysMessage("Campaign data unavailable. Check server logs and restart after resolving the database error.");
            return true;
        }
        handler->PSendSysMessage("Phase: {}", Manager::PhaseName(manager.GetPhase()));
        handler->PSendSysMessage("Alliance supplies: {}", manager.IsComplete(TEAM_ALLIANCE) ? "complete" : "incomplete");
        handler->PSendSysMessage("Horde supplies: {}", manager.IsComplete(TEAM_HORDE) ? "complete" : "incomplete");
        handler->PSendSysMessage("Overall supplies: {}", manager.IsComplete() ? "complete" : "incomplete");
        return true;
    }

    static bool Scores(ChatHandler* handler)
    {
        handler->SendSysMessage("-- Alliance Gathered Resources --");
        handler->SendSysMessage(Manager::Instance().Scores(TEAM_ALLIANCE));
        handler->SendSysMessage("-- Horde Gathered Resources --");
        handler->SendSysMessage(Manager::Instance().Scores(TEAM_HORDE));
        return true;
    }

    static bool Phase(ChatHandler* handler, Tail args)
    {
        std::istringstream input{ std::string(args) };
        std::string name, extra;
        input >> name;
        AQCampaignPhase phase;
        if (name == "disabled") phase = AQ_PHASE_DISABLED;
        else if (name == "effort") phase = AQ_PHASE_WAR_EFFORT;
        else if (name == "ready") phase = AQ_PHASE_READY;
        else if (name == "war") phase = AQ_PHASE_TEN_HOUR_WAR;
        else if (name == "open") phase = AQ_PHASE_OPEN;
        else
        {
            handler->SendSysMessage("Usage: .aqwareffort phase <disabled|effort|ready|war|open>");
            return true;
        }
        if (input >> extra)
        {
            handler->SendSysMessage("Usage: .aqwareffort phase <disabled|effort|ready|war|open>");
            return true;
        }
        auto& manager = Manager::Instance();
        AQCampaignPhase previous = manager.GetPhase();
        if (!manager.SetPhase(phase))
        {
            handler->SendSysMessage("Campaign phase could not be saved. Check server logs and database; restart after resolving the error.");
            return true;
        }
        handler->PSendSysMessage("AQ campaign phase changed: {} -> {}", Manager::PhaseName(previous), Manager::PhaseName(phase));
        return true;
    }
};
}

void AddSC_aq_war_effort()
{
    AQWarEffort::RegisterScarabWallScripts();
    AQWarEffort::RegisterScarabGongScripts();
    new aq_war_effort_world();
    new aq_war_effort_player();
    new aq_war_effort_events();
    new npc_aq_war_effort_quartermaster();
    new aq_war_effort_commands();
}
