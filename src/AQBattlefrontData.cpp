/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * Locations reference AzerothCore mod-war-effort warevent.sql.
 * Regal composition also references current stock Silithus creature templates.
 */
#include "AQBattlefrontData.h"
#include "AQNamedWarBoss.h"
#include <iterator>

namespace AQWarEffort
{
namespace
{
    constexpr uint32 Drone = 15421;
    constexpr uint32 Warbringer = 15758;
    constexpr uint32 AshiColossus = BOSS_COLOSSUS_ASHI;
    // Preserve the proven Ashi entries, positions, orientations and stage roster.
    BattlefrontSpawn const AshiSpawns[] =
    {
        { Drone, { -6497.20f, 1021.79f, 0.38f, 4.00f }, 1 },
        { Drone, { -6540.54f, 985.64f, 0.38f, 3.59f }, 1 },
        { Drone, { -6704.02f, 899.43f, -1.39f, 4.24f }, 2 },
        { Warbringer, { -6610.06f, 924.65f, 0.37f, 3.08f }, 2 },
        { Warbringer, { -6669.62f, 922.71f, -0.69f, 3.68f }, 3 },
        { AshiColossus, { -6458.70f, 1076.01f, -2.90f, 4.05f }, 4 }
    };

    constexpr uint32 RegalSpitfire = 11732;
    constexpr uint32 RegalSlavemaker = 11733;
    constexpr uint32 RegalHiveLord = 11734;
    BattlefrontSpawn const RegalSpawns[] =
    {
        // First four distinct points of reference Regal path 157410. These are
        // stationary runtime emergence slots; no shared template path is installed.
        { RegalSpitfire, { -7870.958496f, 687.510498f, -27.781849f, 0.293172f }, 1 },
        { RegalSpitfire, { -7791.520996f, 727.871521f, -37.473316f, 0.432190f }, 1 },
        { RegalSlavemaker, { -7720.165039f, 719.385498f, -41.306274f, 5.582830f }, 2 },
        { RegalHiveLord, { -7637.914551f, 609.112854f, -51.588173f, 5.230968f }, 2 },
        // Reference Silithus group near Regal (legacy creature GUID 311624).
        { Warbringer, { -7831.444336f, 808.078979f, -9.832852f, 4.501119f }, 3 },
        // Exact historical Regal Colossus emergence (legacy GUID 311616).
        { BOSS_COLOSSUS_REGAL, { -7922.958008f, 625.548523f, -29.006325f, 0.844522f }, 4 }
    };
    static_assert(std::size(AshiSpawns) <= BattlefrontSlotCount);
    static_assert(std::size(RegalSpawns) <= BattlefrontSlotCount);
}

std::array<BattlefrontDefinition, BattlefrontCount> const WarBattlefronts =
{{
    { "Hive'Ashi", "Colossus of Ashi", BOSS_COLOSSUS_ASHI, AshiSpawns },
    { "Hive'Regal", "Colossus of Regal", BOSS_COLOSSUS_REGAL, RegalSpawns }
}};
}
