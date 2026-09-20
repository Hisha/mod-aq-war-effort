#include <thread>
using namespace AQWarEffort;

static WarContentState War(uint64 elapsed, uint32 duration = 300, uint64 epoch = 100000, uint32 id = 7)
{
    return {id, AQ_PHASE_TEN_HOUR_WAR, true, true, epoch, duration, elapsed, duration - elapsed};
}

int main()
{
    auto controller = std::unique_ptr<WarContentController>(new WarContentController);
    auto restart = [&]()
    {
        // A real restart discards runtime objects/ownership but retains the DB.
        TestMap.Creatures.clear(); TestMap.Objects.clear();
        controller.reset(new WarContentController);
    };
    auto retries = [&]()
    {
        for (auto& front : controller->_battlefronts) front.NextAttempt = {};
        controller->_nextBossLoad.fill({}); controller->_nextBossWrite = {};
    };
    auto reconcile = [&](WarContentState state)
    {
        controller->Reconcile(state); TestMap.Flush();
    };
    auto frontGuids = [&](std::size_t index)
    {
        std::vector<uint64> result;
        for (auto const& slot : controller->_battlefronts[index].Slots)
            if (slot.Spawned) result.push_back(slot.Guid.Value);
        return result;
    };
    auto alive = [&](std::size_t index)
    {
        unsigned count = 0;
        for (uint64 guid : frontGuids(index))
            if (Creature* creature = TestMap.GetCreature({guid}); creature && !creature->Removed && creature->Phase) ++count;
        return count;
    };
    auto boss = [&](std::size_t index)
    {
        for (uint64 guid : frontGuids(index))
            if (Creature* creature = TestMap.GetCreature({guid}); creature && creature->Entry == WarBattlefronts[index].BossEntry)
                return creature;
        return static_cast<Creature*>(nullptr);
    };
    auto kill = [&](Creature* actor, WarContentState state)
    {
        assert(actor);
        uint64 guid = actor->Guid.Value;
        controller->OnOwnedBossDeath(actor, state);
        TestMap.Creatures.erase(guid);
    };
    assert(WarBattlefronts.size() == 2 && WarBattlefronts[0].BossEntry == 15742 && WarBattlefronts[1].BossEntry == 15741);
    // Golden Ashi roster: preserve every entry, coordinate, orientation, first stage.
    uint32 const entries[] = {15421,15421,15421,15758,15758,15742};
    Position const places[] = {{-6497.20f,1021.79f,0.38f,4.00f}, {-6540.54f,985.64f,0.38f,3.59f},
        {-6704.02f,899.43f,-1.39f,4.24f}, {-6610.06f,924.65f,0.37f,3.08f},
        {-6669.62f,922.71f,-0.69f,3.68f}, {-6458.70f,1076.01f,-2.90f,4.05f}};
    uint8 const first[] = {1,1,2,2,3,4};
    for (std::size_t i=0; i<6; ++i)
    {
        auto const& row = WarBattlefronts[0].Spawns[i];
        assert(row.Entry == entries[i] && row.FirstStage == first[i]);
        assert(row.Location.x == places[i].x && row.Location.y == places[i].y);
        assert(row.Location.z == places[i].z && row.Location.o == places[i].o);
    }
    for (uint32 duration : {300u, 36000u})
    {
        restart();
        for (uint8 stage = 1; stage <= 4; ++stage)
        {
            auto state = War(duration * (stage-1)/4, duration);
            reconcile(state);
            assert(controller->_battlefronts[0].Stage == stage && controller->_battlefronts[1].Stage == stage);
            unsigned expected = stage == 1 ? 2 : stage == 2 ? 4 : stage == 3 ? 5 : 6;
            assert(alive(0) == expected && alive(1) == expected);
            assert(TestMap.Count(15741) == (stage == 4 ? 1u : 0u));
            assert(TestMap.Count(15742) == (stage == 4 ? 1u : 0u));
            assert(TestMap.Count(15740) == 0);
            assert(TestMap.Objects.size() == 1);
            auto created = TestMap.CreatureCreates, reads = CharacterDatabase.Reads, logs = InfoLogs;
            for (unsigned i = 0; i < 100; ++i) { retries(); reconcile(state); }
            assert(TestMap.CreatureCreates == created && CharacterDatabase.Reads == reads && InfoLogs == logs);
            if (stage < 4)
            {
                auto other = frontGuids(1);
                kill(TestMap.GetCreature({frontGuids(0)[0]}), state); retries(); reconcile(state);
                assert(alive(0) == expected-1 && alive(1) == expected && frontGuids(1) == other);
                auto ashi = frontGuids(0);
                kill(TestMap.GetCreature({frontGuids(1)[0]}), state); retries(); reconcile(state);
                assert(alive(1) == expected-1 && frontGuids(0) == ashi);
                assert(CharacterDatabase.Kills.empty());
            }
        }
    }
    restart(); auto state = War(180); reconcile(state);
    assert(alive(0) == 5 && alive(1) == 5 && !boss(0) && !boss(1));
    restart(); controller->Reconcile(state, true); TestMap.Flush();
    assert(alive(0) == 5 && alive(1) == 5); // Direct stage 3, no prior-stage replay.

    // Partial failure on Regal never rebuilds/removes Ashi.
    restart(); ObjectManager.Missing.insert(11734); reconcile(state);
    assert(alive(0) == 5 && alive(1) == 4); auto ashi = frontGuids(0);
    ObjectManager.Missing.clear(); retries(); reconcile(state);
    assert(alive(1) == 5 && frontGuids(0) == ashi);

    for (unsigned mask = 0; mask < 4; ++mask)
    {
        CharacterDatabase.Kills.clear(); restart(); state = War(225, 300, 200000+mask);
        reconcile(state); assert(boss(0) && boss(1)); // Both read independently on first tick.
        for (unsigned index = 0; index < 2; ++index)
            if (mask & (1u << index))
            {
                Creature copy = *boss(index);
                kill(boss(index), state);
                auto rows = CharacterDatabase.Kills;
                controller->OnOwnedBossDeath(&copy, state); // Duplicate callback must be idempotent.
                assert(CharacterDatabase.Kills == rows);
                assert(rows.contains({state.CampaignId,state.Origin,WarBattlefronts[index].BossEntry}));
            }
        retries(); reconcile(state);
        assert(bool(boss(0)) == !(mask & 1) && bool(boss(1)) == !(mask & 2));
        restart(); controller->Reconcile(state, true); TestMap.Flush();
        assert(bool(boss(0)) == !(mask & 1) && bool(boss(1)) == !(mask & 2));
        assert(alive(0) == ((mask&1) ? 5u : 6u) && alive(1) == ((mask&2) ? 5u : 6u));
        auto rows = CharacterDatabase.Kills;
        ++state.Origin; reconcile(state); assert(boss(0) && boss(1));
        assert(CharacterDatabase.Kills == rows); // New war permits both; history retained.
    }
    CharacterDatabase.Kills.clear(); restart(); state = War(225); reconcile(state);
    Creature alien = *boss(1); alien.Guid.Value += 90000;
    controller->OnOwnedBossDeath(&alien,state);
    controller->OnOwnedBossDeath(boss(1),War(225,300,state.Origin+1));
    controller->OnOwnedBossDeath(boss(1),War(225,300,state.Origin,state.CampaignId+1));
    assert(CharacterDatabase.Kills.empty());
    Creature unrelated; unrelated.Entry=15740; unrelated.Guid.Value=99999;
    controller->OnOwnedBossDeath(&unrelated,state); assert(CharacterDatabase.Kills.empty());
    // Concurrent callbacks for two independent owned bosses retain both keys.
    std::thread a([&] { controller->OnOwnedBossDeath(boss(0), state); });
    std::thread b([&] { controller->OnOwnedBossDeath(boss(1), state); });
    a.join(); b.join(); assert(CharacterDatabase.Kills.size() == 2);

    CharacterDatabase.Kills.clear(); restart(); CharacterDatabase.FailRead.insert(15742);
    reconcile(state); assert(!boss(0) && boss(1)); // Ashi read failure cannot throttle Regal.
    auto regal = frontGuids(1); CharacterDatabase.FailRead.clear(); retries(); reconcile(state);
    assert(boss(0) && frontGuids(1) == regal);
    restart(); CharacterDatabase.FailRead.insert(15741); reconcile(state);
    assert(boss(0) && !boss(1));
    CharacterDatabase.FailRead.clear(); retries(); reconcile(state); assert(boss(1));
    CharacterDatabase.FailWrite.insert(15741);
    kill(boss(1),state); assert(!CharacterDatabase.Kills.contains({7,100000,15741}));
    retries(); reconcile(state); assert(!boss(1) && boss(0));
    // A new epoch must not inherit an old pending death, which still retries its original key.
    ++state.Origin; reconcile(state); assert(boss(1) && boss(0));
    CharacterDatabase.FailWrite.clear(); retries(); reconcile(state);
    assert(CharacterDatabase.Kills.contains({7,100000,15741}));
    assert(!CharacterDatabase.Kills.contains({7,100001,15741}));
    restart(); reconcile(state); assert(boss(1) && boss(0));

    // Cleanup hides/removes both fronts, but never touches an unrelated same-entry actor.
    auto& foreign = TestMap.Creatures[900000]; foreign.Entry=15741; foreign.Guid.Value=900000;
    for (auto phase : {AQ_PHASE_OPEN,AQ_PHASE_READY,AQ_PHASE_WAR_EFFORT,AQ_PHASE_DISABLED})
    {
        reconcile(state); auto inactive=state; inactive.Phase=phase; reconcile(inactive);
        assert(TestMap.Creatures.size() == 1 && TestMap.Creatures.contains(900000));
        assert(TestMap.Objects.empty());
    }
    restart(); auto inactive=state; inactive.Phase=AQ_PHASE_OPEN;
    controller->Reconcile(inactive,true); assert(TestMap.Creatures.empty() && TestMap.Objects.empty());
    reconcile(state); inactive=state; inactive.Managed=false; reconcile(inactive);
    assert(TestMap.Creatures.empty() && TestMap.Objects.empty());

    // Exercise the actual registered unit-death observer, too.
    CharacterDatabase.Kills.clear(); TestState=War(225,300,800000);
    auto& singleton=WarContentController::Instance(); singleton.Reconcile(TestState);
    Creature* regalBoss = nullptr;
    for (auto& [guid, actor] : TestMap.Creatures) if(actor.Entry==15741) regalBoss=&actor;
    assert(regalBoss); aq_named_war_boss_death hook; hook.OnUnitDeath(regalBoss,nullptr);
    assert(CharacterDatabase.Kills.contains({7,800000,15741}));
    TestState.Phase=AQ_PHASE_OPEN; singleton.Reconcile(TestState); TestMap.Flush();
    assert(TestMap.Creatures.empty() && TestMap.Objects.empty());
}
