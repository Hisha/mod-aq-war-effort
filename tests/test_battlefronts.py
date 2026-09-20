"""Exercise actual battlefront data/controller against isolated core/map/DB doubles."""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
root = Path(__file__).resolve().parents[1]
with TemporaryDirectory(prefix='aq-battlefronts-') as directory:
    tmp = Path(directory)
    (tmp / 'Define.h').write_text('#pragma once\n#include <cstdint>\nusing uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;\n')
    (tmp / 'ObjectGuid.h').write_text('#pragma once\n#include "Define.h"\nstruct ObjectGuid { uint64 Value=0; void Clear(){Value=0;} bool operator==(ObjectGuid const&)const=default; };\n')
    (tmp / 'Position.h').write_text('#pragma once\nstruct Position { float x=0,y=0,z=0,o=0; float GetPositionX()const{return x;} float GetPositionY()const{return y;} };\n')
    (tmp / 'EventMap.h').write_text('#pragma once\nstruct EventMap { void Reset(){} };\n')
    for name in ['Creature', 'DatabaseEnv', 'Field', 'GameObject', 'Log', 'Map', 'MapMgr', 'ObjectMgr', 'QueryResult', 'TemporarySummon', 'UnitScript']:
        (tmp / (name + '.h')).write_text('#include "battlefront_core_stubs.h"\n')
    unit = tmp / 'battlefronts.cpp'
    unit.write_text('#include "battlefront_core_stubs.h"\n#include "AQBattlefrontData.cpp"\n#include "AQWarContent.cpp"\n' + (root / 'tests/battlefront_regression.cpp').read_text())
    exe = tmp / 'battlefronts'
    subprocess.run(['c++', '-std=c++20', '-pthread', '-I'+str(tmp), '-I'+str(root/'tests'), '-I'+str(root/'src'), str(unit), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    for name in ['test_battlefront_stage', 'test_named_war_boss_rules']:
        executable = tmp / name
        subprocess.run(['c++', '-std=c++20', '-I'+str(root/'src'), str(root/('tests/'+name+'.cpp')), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        print('PASS:', name)
print('PASS: actual shared controller, both rosters, ownership, stages, boss persistence/failure/restart matrix and crystal')
