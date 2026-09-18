#include "AQBattlefrontStage.h"
#include <cassert>
#include <initializer_list>

using AQWarEffort::BattlefrontStage;

int main()
{
    for (auto duration : { 300u, 36000u })
    {
        assert(BattlefrontStage(0, duration) == 1);
        assert(BattlefrontStage(duration / 4 - 1, duration) == 1);
        assert(BattlefrontStage(duration / 4, duration) == 2);
        assert(BattlefrontStage(duration / 2 - 1, duration) == 2);
        assert(BattlefrontStage(duration / 2, duration) == 3);
        assert(BattlefrontStage(duration * 3 / 4 - 1, duration) == 3);
        assert(BattlefrontStage(duration * 3 / 4, duration) == 4);
        assert(BattlefrontStage(duration - 1, duration) == 4);
        assert(BattlefrontStage(duration, duration) == 0);
    }
    // Direct reconstruction after downtime selects the current stage.
    assert(BattlefrontStage(165, 300) == 3);
    assert(BattlefrontStage(21600, 36000) == 3);
    assert(BattlefrontStage(0, 0) == 0);
}
