"""Compile actual Manager code with deterministic clock and isolated core/DB test doubles.
Run from any directory: python3 tests/test_campaign.py. Requires C++20 compiler.
World hook wiring is checked separately; no running realm/database is contacted.
"""
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
root = Path(__file__).resolve().parents[1]
source = (root / 'src/mod_aq_war_effort.cpp').read_text()
assert 'InitializeGong();\n    ReconcileWarTime();' in source
assert 'Manager::Instance().UpdateGong(diff);\n        Manager::Instance().ReconcileWarTime();' in source
assert 'Manager::Instance().LoadWarDuration();\n            Manager::Instance().ReconcileWarTime();' in source
assert 'manager.WarTimeStatus()' in source
assert 'WarContentController::Instance().Reconcile(Manager::Instance().GetWarContentState(), true);' in source
world_update = source[source.index('void OnUpdate(uint32 diff) override'):source.index('void OnAfterConfigLoad(bool reload) override')]
assert world_update.index('UpdateGong') < world_update.index('ReconcileWarTime') < world_update.index('WarContentController')
# Shared content behavior is exercised by test_battlefronts.py.
manager = source[source.index('namespace AQWarEffort'):source.index('\nnamespace\n{\nusing namespace AQWarEffort;')]
# Replace ONLY the clock read in compiled source, including EnterPhase, for deterministic restart tests.
manager = manager.replace('std::time(nullptr)', 'testNow')
stubs = (root / 'tests/manager_test_stubs.h').read_text()
# Gong recovery/wall behavior are covered by the independent actual gong/wall regression.
boundaries = '\nnamespace AQWarEffort { void Manager::SyncWall() {} void Manager::InitializeGong() {} }\n'
with TemporaryDirectory(prefix='aq-campaign-test-') as directory:
    temp = Path(directory)
    (temp / 'Define.h').write_text('#pragma once\n#include <cstdint>\nusing uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;\n')
    (temp / 'EventMap.h').write_text('#pragma once\nclass EventMap { public: void Reset() {} };\n')
    (temp / 'ObjectGuid.h').write_text('#pragma once\nstruct ObjectGuid { uint64 value=0; void Clear(){value=0;} };\n')
    (temp / 'Position.h').write_text('#pragma once\nstruct Position { float x=0,y=0,z=0,o=0; float GetPositionX()const{return x;} float GetPositionY()const{return y;} };\n')
    for name in ('collection_regression', 'war_timer_regression'):
        cpp = temp / (name + '.cpp')
        cpp.write_text(stubs + manager + boundaries + (root / ('tests/' + name + '.cpp')).read_text())
        executable = temp / name
        subprocess.run(['c++', '-std=c++20', '-I' + str(temp), '-I' + str(root / 'src'), str(cpp), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        print('PASS:', name)
