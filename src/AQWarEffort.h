/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * Material/quest mappings derived from AzerothCore mod-war-effort.
 */
#ifndef MOD_AQ_WAR_EFFORT_H
#define MOD_AQ_WAR_EFFORT_H

#include "Define.h"
#include <array>
#include <string>
#include <mutex>

class Player;
class Quest;
class GameObject;

enum AQCampaignPhase : uint8
{
    AQ_PHASE_DISABLED = 0,
    AQ_PHASE_WAR_EFFORT = 1,
    AQ_PHASE_READY = 2,
    AQ_PHASE_TEN_HOUR_WAR = 3,
    AQ_PHASE_OPEN = 4
};

namespace AQWarEffort
{
    constexpr uint8 FactionCount = 2;
    constexpr uint8 MaterialCount = 15;
    constexpr uint32 QuestBangGong = 8743;
    constexpr uint32 QuartermasterHorde = 15700;
    constexpr uint32 QuartermasterAlliance = 15701;

    enum MaterialCategory : uint8
    {
        MATERIAL_CAT_BANDAGES,
        MATERIAL_CAT_FOOD,
        MATERIAL_CAT_HERBS,
        MATERIAL_CAT_METAL,
        MATERIAL_CAT_LEATHER,
        MATERIAL_CAT_COUNT
    };

    struct Material
    {
        char const* Name;
        uint32 Quest;
        uint32 RepeatQuest;
        MaterialCategory Category;
        uint32 ItemId;
        uint32 TurnInQuantity;
        char const* GoalKey;
    };

    struct Campaign
    {
        AQCampaignPhase Phase{ AQ_PHASE_WAR_EFFORT };
        uint64 PhaseStartedAt{ 0 };
        uint64 GongRungAt{ 0 };
        uint64 OpenedAt{ 0 };
        // Stored as actual contributed material quantities (data version 2).
        std::array<std::array<uint64, MaterialCount>, FactionCount> Contributions{};
        bool operator==(Campaign const&) const = default;
    };

    void RegisterScarabWallScripts();
    void RegisterScarabGongScripts();

    class Manager
    {
    public:
        static Manager& Instance();
        void Initialize();
        bool IsEnabled() const { std::lock_guard lock(_mutex); return _enabled; }
        bool IsAvailable() const { std::lock_guard lock(_mutex); return _loaded; }
        uint32 GetId() const { return _id; }
        AQCampaignPhase GetPhase() const { std::lock_guard lock(_mutex); return _campaign.Phase; }
        static char const* PhaseName(AQCampaignPhase phase);
        static Material const& GetMaterial(uint8 faction, uint8 material);
        bool IsComplete(uint8 faction) const;
        bool IsComplete() const;
        std::string Scores(uint8 faction, uint8 category = MATERIAL_CAT_COUNT) const;
        bool SetPhase(AQCampaignPhase phase);
        void OnQuestReward(Player* player, Quest const* quest);
        void SyncCollectionEvent();
        void SyncWall();
        // One lock also serializes PROCESS_INPLACE gong packets across map workers.
        std::recursive_mutex& Mutex() const { return _mutex; }
        void InitializeGong();
        bool PrepareGong(Player* player, GameObject* go);
        void ObserveGongReward(Player* player, GameObject* go);
        void UpdateGong(uint32 diff);
        bool GongAvailable() const;
        void BeginWallCeremony();
        void UpdateWallCeremony(uint32 diff);
        uint32 WallCeremonyElapsed() const { return _wallCeremonyElapsed; }
        bool WallCeremonyActive() const { return _wallCeremony; }

    private:
        Manager() = default;
        bool ValidateConfiguration();
        static bool ValidateQuest(Material const& material, Quest const* quest);
        bool ReadCampaign(Campaign& campaign) const;
        bool Persist(Campaign const& next);
        bool IsComplete(Campaign const& campaign, uint8 faction) const;
        static void EnterPhase(Campaign& campaign, AQCampaignPhase phase);

        mutable std::recursive_mutex _mutex;
        bool _gongHealthy{ false };
        bool _gongPending{ false };
        bool _gongObserved{ false };
        uint32 _gongPlayer{ 0 };
        bool _wallCeremony{ false };
        uint32 _wallCeremonyElapsed{ 0 };
        bool _initialized{ false };
        bool _syncingEvent{ false };
        bool _enabled{ false };
        bool _loaded{ false };
        uint32 _id{ 1 };
        Campaign _campaign;
        std::array<std::array<uint64, MaterialCount>, FactionCount> _goals{};
    };
}
#endif
