
int main()
{
    using namespace AQWarEffort;
    auto& m = Manager::Instance();
    for (uint8 f = 0; f < FactionCount; ++f)
        for (uint8 i = 0; i < MaterialCount; ++i)
        {
            auto const& mat = m.GetMaterial(f, i);
            for (uint32 q : {mat.Quest, mat.RepeatQuest})
                objectMgr.quests[q] = Quest{q, {mat.ItemId}, {mat.TurnInQuantity}};
        }
    events.changed = [&]() { m.SyncCollectionEvent(); };
    auto seed = [&](uint64 start, uint64 gong)
    {
        Campaign c;
        c.Phase = AQ_PHASE_TEN_HOUR_WAR;
        c.PhaseStartedAt = start;
        c.GongRungAt = gong;
        c.OpenedAt = 123;
        for (uint8 f = 0; f < FactionCount; ++f)
            for (uint8 i = 0; i < MaterialCount; ++i)
                c.Contributions[f][i] = 100 + 30 * f + i;
        CharacterDatabase.rows[1] = c;
        CharacterDatabase.hasCampaign[1] = true;
        m.Initialize();
        return c;
    };
    testNow = 1000000;
    auto original = seed(testNow - 8251, testNow - 8251);
    assert(m.GetPhase() == AQ_PHASE_TEN_HOUR_WAR);
    assert(m.WarTimeStatus() == "Ten Hour War duration: 10:00:00\nElapsed: 02:17:31\nRemaining: 07:42:29\nTimer origin: Scarab Gong");
    int commits = CharacterDatabase.commits;
    for (int i = 0; i < 50; ++i) m.ReconcileWarTime();
    assert(CharacterDatabase.commits == commits); // No countdown writes.
    testNow += 3600;
    m.Initialize(); // Simulated shutdown: fresh DB read, no uptime carried forward.
    assert(m.WarTimeStatus().find("Remaining: 06:42:29") != std::string::npos);
    assert(CharacterDatabase.rows[1] == original);
    testNow = original.GongRungAt + 36000;
    m.Initialize(); // Offline deadline reconciliation.
    auto opened = CharacterDatabase.rows[1];
    assert(m.GetPhase() == AQ_PHASE_OPEN && opened.Phase == AQ_PHASE_OPEN);
    assert(opened.OpenedAt == uint64(testNow) && opened.PhaseStartedAt == uint64(testNow));
    assert(opened.GongRungAt == original.GongRungAt && opened.Contributions == original.Contributions);
    assert(!events.active && m.WarTimeStatus().empty());
    ++testNow; m.Initialize(); assert(CharacterDatabase.rows[1] == opened);

    // Administrative war retains old gong history but starts now.
    assert(m.SetPhase(AQ_PHASE_TEN_HOUR_WAR));
    auto admin = CharacterDatabase.rows[1];
    assert(admin.GongRungAt == original.GongRungAt && admin.PhaseStartedAt == uint64(testNow));
    assert(m.WarTimeStatus().find("Administrative phase start") != std::string::npos);
    assert(m.WarTimeStatus().find("Remaining: 10:00:00") != std::string::npos);
    testNow += 100;
    assert(m.SetPhase(AQ_PHASE_TEN_HOUR_WAR)); // Same phase cannot extend deadline.
    assert(CharacterDatabase.rows[1] == admin);
    m.Initialize(); assert(CharacterDatabase.rows[1] == admin);
    testNow = admin.PhaseStartedAt + 36000;
    m.ReconcileWarTime(); // Runtime expiration.
    assert(m.GetPhase() == AQ_PHASE_OPEN && CharacterDatabase.rows[1].OpenedAt == uint64(testNow));
    assert(CharacterDatabase.rows[1].GongRungAt == admin.GongRungAt);
    assert(CharacterDatabase.rows[1].Contributions == admin.Contributions);

    config.duration = "300";
    auto shortWar = seed(testNow, 0);
    testNow += 299; m.ReconcileWarTime(); assert(m.GetPhase() == AQ_PHASE_TEN_HOUR_WAR);
    ++testNow; m.ReconcileWarTime(); assert(m.GetPhase() == AQ_PHASE_OPEN);
    config.duration = "36000";
    auto changing = seed(testNow - 400, testNow - 400);
    assert(m.GetPhase() == AQ_PHASE_TEN_HOUR_WAR);
    config.duration = "300"; m.Initialize();
    assert(m.GetPhase() == AQ_PHASE_OPEN && CharacterDatabase.rows[1].GongRungAt == changing.GongRungAt);
    config.duration = "36000"; seed(testNow - 400, 0);
    config.duration = "300"; m.LoadWarDuration(); m.ReconcileWarTime();
    assert(m.GetPhase() == AQ_PHASE_OPEN); // Live reload uses original origin too.
    assert(m.FormatDuration(90061) == "25:01:01");
    assert(m.FormatDuration(0) == "00:00:00");
    config.duration = "90061"; seed(testNow, 0);
    assert(m.WarTimeStatus().find("25:01:01") != std::string::npos);

    for (auto bad : {"", "0", "-1", "+1", "1.5", "abc", "300s", "4294967296", "18446744073709551616"})
    {
        config.duration = bad; m.LoadWarDuration();
        testNow += 100000; m.ReconcileWarTime();
        assert(m.IsAvailable() && m.GetPhase() == AQ_PHASE_TEN_HOUR_WAR);
        assert(m.WarTimeStatus().find("invalid") != std::string::npos);
    }
    config.duration = "4294967295"; m.LoadWarDuration(); assert(m._warDuration == 4294967295u);
    config.duration = "300"; m.LoadWarDuration();
    auto future = seed(testNow + 500, 0);
    m.ReconcileWarTime(); assert(CharacterDatabase.rows[1] == future);
    assert(m.WarTimeStatus().find("Elapsed: 00:00:00") != std::string::npos);
    seed(0, 0); assert(m.WarTimeStatus().find("missing persisted") != std::string::npos);
    config.enabled = false;
    auto disabled = seed(testNow - 500, 0); assert(CharacterDatabase.rows[1] == disabled);
    assert(m.WarTimeStatus().find("module disabled") != std::string::npos);
    config.enabled = true;
    config.duration = "36000"; seed(testNow, 0);
    m._gongPending = true;
    assert(!m.SetPhase(AQ_PHASE_OPEN)); testNow += 36000; m.ReconcileWarTime();
    assert(m.GetPhase() == AQ_PHASE_TEN_HOUR_WAR);
    m._gongPending = false;
    CharacterDatabase.failWrite = true; m.ReconcileWarTime();
    assert(!m.IsAvailable() && CharacterDatabase.rows[1].Phase == AQ_PHASE_TEN_HOUR_WAR);
    assert(m.GetPhase() != AQ_PHASE_OPEN); // Failed write never publishes OPEN.
    CharacterDatabase.failWrite = false; m.Initialize(); assert(m.GetPhase() == AQ_PHASE_OPEN);
    config.duration = "300";
    seed(testNow, testNow); // Same-second admin/gong collision has identical origin/deadline.
    assert(m.SetPhase(AQ_PHASE_READY)); assert(m.SetPhase(AQ_PHASE_TEN_HOUR_WAR));
    testNow += 300; m.ReconcileWarTime(); assert(m.GetPhase() == AQ_PHASE_OPEN);
    seed(testNow, 0); assert(m.SetPhase(AQ_PHASE_OPEN)); assert(m.GetPhase() == AQ_PHASE_OPEN);
}
