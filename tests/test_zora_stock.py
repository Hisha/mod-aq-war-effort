"""Read-only audit of current stock templates and the historical Zora placements."""
import argparse
import csv
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--core', type=Path, required=True)
parser.add_argument('--reference', type=Path, required=True)
args = parser.parse_args()
base = args.core / 'data/sql/base/db_world'
sql = (base / 'creature_template.sql').read_text()
columns = re.findall(r'^  `([^`]+)`', sql, re.M)
expected = {11725: "Hive'Zora Waywatcher", 11726: "Hive'Zora Tunneler",
            11727: "Hive'Zora Wasp", 11728: "Hive'Zora Reaver",
            11729: "Hive'Zora Hive Sister", 15740: 'Colossus of Zora'}
found = {}
for line in sql.splitlines():
    if not any(line.startswith('('+str(entry)+',') for entry in expected):
        continue
    values = next(csv.reader([line.rstrip(',;')[1:-1]], quotechar="'", escapechar='\\'))
    assert len(values) == len(columns)
    row = dict(zip(columns, values))
    entry = int(row['entry'])
    assert row['name'] == expected[entry], row
    assert not row['ScriptName'], row
    if entry in (11727, 11728, 11729, 15740):
        assert row['AIName'] == 'SmartAI', row
    found[entry] = row
assert set(found) == set(expected)
assert found[15740]['lootid'] == '0'
assert found[15740]['HealthModifier'] == '1000'
reference = (args.reference / 'data/sql/db-world/updates/warevent.sql').read_text()
path = reference.split('SET @NPC := 15740;')[1].split('DELETE FROM `creature_template_addon`')[0]
assert 'SET @PATH := @NPC * 10;' in path
points = re.findall(r'\(@PATH,\s*\d+,\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(-?[\d.]+),', path)
assert len(points) == 11 and points[6:] == list(reversed(points[:5]))
positions = points[:5]
for guid, entry in ((311617, 15740),):
    match = re.search(r'\('+str(guid)+','+str(entry)+r',1,0,0,1,1,0,\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),', reference)
    assert match, guid
    positions.append(match.groups())
source = (Path(__file__).resolve().parents[1] / 'src/AQBattlefrontData.cpp').read_text()
zora = source.split('BattlefrontSpawn const ZoraSpawns[] =')[1].split('};', 1)[0]
actual = re.findall(r'\{ (-?[\d.]+)f, (-?[\d.]+)f, (-?[\d.]+)f, (-?[\d.]+)f \}', zora)
assert [tuple(map(float, p)) for p in actual] == [tuple(map(float, p)) for p in positions]
smart = (base / 'smart_scripts.sql').read_text()
for entry, spell in ((11727, 19448), (11728, 16790), (11728, 40504), (11729, 7951), (15740, 26167)):
    assert any(line.startswith('('+str(entry)+',0,') and ','+str(spell)+',' in line
               for line in smart.splitlines()), (entry, spell)
print('PASS stock identities/AI, Zora path 157400 and exact six reference positions/orientations')
