"""Read-only audit of current stock templates and the historical Regal placements."""
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
expected = {11730: "Hive'Regal Ambusher", 11731: "Hive'Regal Burrower",
            11732: "Hive'Regal Spitfire", 11733: "Hive'Regal Slavemaker",
            11734: "Hive'Regal Hive Lord", 15741: 'Colossus of Regal',
            15742: 'Colossus of Ashi', 15758: 'Supreme Anubisath Warbringer'}
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
    if entry in (11732, 11733, 11734, 15741):
        assert row['AIName'] == 'SmartAI', row
    found[entry] = row
assert set(found) == set(expected)
reference = (args.reference / 'data/sql/db-world/updates/warevent.sql').read_text()
path = reference.split('SET @NPC := 15741;')[1].split('DELETE FROM `creature_template_addon`')[0]
assert 'SET @PATH := @NPC * 10;' in path
points = re.findall(r'\(@PATH,\s*\d+,\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(-?[\d.]+),', path)
assert len(points) == 7 and points[4:] == [points[2], points[1], points[0]]
positions = points[:4]
for guid, entry in ((311624, 15758), (311616, 15741)):
    match = re.search(r'\('+str(guid)+','+str(entry)+r',1,0,0,1,1,0,\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),', reference)
    assert match, guid
    positions.append(match.groups())
source = (Path(__file__).resolve().parents[1] / 'src/AQBattlefrontData.cpp').read_text()
regal = source.split('BattlefrontSpawn const RegalSpawns[] =')[1].split('static_assert')[0]
actual = re.findall(r'\{ (-?[\d.]+)f, (-?[\d.]+)f, (-?[\d.]+)f, (-?[\d.]+)f \}', regal)
assert [tuple(map(float, p)) for p in actual] == [tuple(map(float, p)) for p in positions]
smart = (base / 'smart_scripts.sql').read_text()
for entry, spell in ((11732, 21047), (11732, 5708), (11733, 3584), (11733, 19469),
                     (11734, 19471), (15741, 26167)):
    assert any(line.startswith('('+str(entry)+',0,') and ','+str(spell)+',' in line
               for line in smart.splitlines()), (entry, spell)
print('PASS stock identities/AI, Regal path 157410 and exact six reference positions/orientations')
