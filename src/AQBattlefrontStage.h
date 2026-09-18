/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef MOD_AQ_BATTLEFRONT_STAGE_H
#define MOD_AQ_BATTLEFRONT_STAGE_H

#include <cstdint>

namespace AQWarEffort
{
    // Stage is reconstructed solely from persisted war origin and duration.
    inline std::uint8_t BattlefrontStage(std::uint64_t elapsed, std::uint32_t duration)
    {
        if (!duration || elapsed >= duration)
            return 0;
        if (elapsed * 4 >= std::uint64_t(duration) * 3) return 4;
        if (elapsed * 2 >= duration) return 3;
        if (elapsed * 4 >= duration) return 2;
        return 1;
    }
}
#endif
