/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef MOD_AQ_NAMED_WAR_BOSS_H
#define MOD_AQ_NAMED_WAR_BOSS_H

#include <cstdint>

namespace AQWarEffort
{
    enum NamedWarBoss : std::uint32_t
    {
        BOSS_COLOSSUS_ZORA = 15740,
        BOSS_COLOSSUS_REGAL = 15741,
        BOSS_COLOSSUS_ASHI = 15742
    };

    struct WarBossKey
    {
        std::uint32_t CampaignId;
        std::uint64_t Origin;
        std::uint32_t BossId;
        bool operator==(WarBossKey const&) const = default;
    };

    inline bool IsNamedWarBoss(std::uint32_t entry)
    {
        return entry == BOSS_COLOSSUS_ZORA || entry == BOSS_COLOSSUS_REGAL
            || entry == BOSS_COLOSSUS_ASHI;
    }

    template <class Guid>
    bool IsOwnedWarBossDeath(WarBossKey const& death, WarBossKey const& active,
        Guid const& deathGuid, Guid const& ownedGuid, bool slotSpawned, bool stageActive)
    {
        return stageActive && slotSpawned && IsNamedWarBoss(death.BossId)
            && death == active && deathGuid == ownedGuid;
    }
}
#endif
