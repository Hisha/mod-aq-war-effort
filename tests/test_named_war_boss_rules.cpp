#include "AQNamedWarBoss.h"
#include <cassert>
#include <set>
#include <tuple>

using namespace AQWarEffort;

int main()
{
    static_assert(BOSS_COLOSSUS_ZORA == 15740);
    static_assert(BOSS_COLOSSUS_REGAL == 15741);
    static_assert(BOSS_COLOSSUS_ASHI == 15742);
    assert(IsNamedWarBoss(15742) && IsNamedWarBoss(15741) && IsNamedWarBoss(15740));
    assert(!IsNamedWarBoss(15421));

    WarBossKey const adminWar{ 7, 2000, BOSS_COLOSSUS_ASHI };
    assert(IsOwnedWarBossDeath(adminWar, adminWar, 101u, 101u, true, true));
    assert(!IsOwnedWarBossDeath(adminWar, adminWar, 102u, 101u, true, true));
    assert(!IsOwnedWarBossDeath(adminWar, adminWar, 101u, 101u, false, true));
    assert(!IsOwnedWarBossDeath(adminWar, adminWar, 101u, 101u, true, false));
    assert(!IsOwnedWarBossDeath({ 8, 2000, BOSS_COLOSSUS_ASHI }, adminWar, 101u, 101u, true, true));
    assert(!IsOwnedWarBossDeath({ 7, 2001, BOSS_COLOSSUS_ASHI }, adminWar, 101u, 101u, true, true));
    assert(!IsOwnedWarBossDeath({ 7, 2000, BOSS_COLOSSUS_REGAL }, adminWar, 101u, 101u, true, true));

    // A historical gong timestamp cannot identify a later admin war.
    WarBossKey const gongWar{ 7, 1000, BOSS_COLOSSUS_ASHI };
    assert(!(gongWar == adminWar));
    std::set<std::tuple<std::uint32_t, std::uint64_t, std::uint32_t>> keys;
    auto insert = [&keys](WarBossKey const& key)
    {
        return keys.emplace(key.CampaignId, key.Origin, key.BossId).second;
    };
    assert(insert(gongWar));
    assert(!insert(gongWar)); // The schema's natural key rejects duplicates.
    assert(insert(adminWar));
    assert(keys.size() == 2);
}
