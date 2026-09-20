/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef MOD_AQ_BATTLEFRONT_DATA_H
#define MOD_AQ_BATTLEFRONT_DATA_H

#include "Define.h"
#include "Position.h"
#include <array>
#include <span>

namespace AQWarEffort
{
    constexpr std::size_t BattlefrontCount = 3;
    constexpr std::size_t BattlefrontSlotCount = 8;

    struct BattlefrontSpawn
    {
        uint32 Entry;
        Position Location;
        uint8 FirstStage;
    };

    struct BattlefrontDefinition
    {
        char const* Name;
        char const* BossName;
        uint32 BossEntry;
        std::span<BattlefrontSpawn const> Spawns;
    };

    extern std::array<BattlefrontDefinition, BattlefrontCount> const WarBattlefronts;
}
#endif
