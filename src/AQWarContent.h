/*
 * Copyright (C) 2026 Kevin Smith
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef MOD_AQ_WAR_CONTENT_H
#define MOD_AQ_WAR_CONTENT_H

#include "AQWarEffort.h"
#include "ObjectGuid.h"
#include <chrono>

namespace AQWarEffort
{
    // A coherent read-only view of the persisted campaign and current timing policy.
    // Later scenes can select stages with Progress(), independent of real-hour lengths.
    struct WarContentState
    {
        uint32 CampaignId{ 0 };
        AQCampaignPhase Phase{ AQ_PHASE_DISABLED };
        bool Managed{ false };
        bool TimingValid{ false };
        uint64 Origin{ 0 };
        uint32 Duration{ 0 };
        uint64 Elapsed{ 0 };
        uint64 Remaining{ 0 };

        double Progress() const
        {
            if (!TimingValid || !Duration)
                return 0.0;
            return Elapsed >= Duration ? 1.0 : double(Elapsed) / double(Duration);
        }

        bool Active() const
        {
            return Managed && Phase == AQ_PHASE_TEN_HOUR_WAR && TimingValid && Remaining != 0;
        }
    };

    // Called only at startup and the world-update boundary after map workers finish.
    // Runtime GUIDs are ownership handles, never persisted spawn/progression state.
    class WarContentController
    {
    public:
        static WarContentController& Instance();
        void Reconcile(WarContentState const& state, bool startup = false);

    private:
        WarContentController() = default;
        WarContentController(WarContentController const&) = delete;
        WarContentController& operator=(WarContentController const&) = delete;
        void Cleanup();
        ObjectGuid _crystal;
        uint32 _campaignId{ 0 };
        uint64 _origin{ 0 };
        bool _active{ false };
        bool _failureLogged{ false };
        std::chrono::steady_clock::time_point _nextAttempt{};
    };
}
#endif
