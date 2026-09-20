"""Test the existing schema and exact controller queries in an isolated MySQL instance."""
import argparse
import json
import os
import re
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory

parser = argparse.ArgumentParser()
parser.add_argument('--mysql-root', type=Path, required=True)
args = parser.parse_args()
module = Path(__file__).resolve().parents[1]
source = (module / 'src/AQWarContent.cpp').read_text()
block = source.split('bool WarContentController::PersistBossKill(')[1].split('void WarContentController::RetryBossKills')[0]
def query_after(token):
    part = block.split(token)[1].split('campaignId, origin, entry')[0]
    return ''.join(json.loads(s) for s in re.findall(r'"(?:[^"\\]|\\.)*"', part))
insert = query_after('CharacterDatabase.DirectExecute(')
select = query_after('CharacterDatabase.Query(')
statements = ['CREATE DATABASE aq_boss_test', 'USE aq_boss_test']
def add(s):
    statements.append(s.strip().rstrip(';'))
def check(expression, message):
    add(f"CALL assert_true(({expression}), '{message}')")
schema = (module / 'data/sql/db-characters/base/003_aq_named_war_boss_kills.sql').read_text()
schema = '\n'.join(line for line in schema.splitlines() if not line.strip().startswith('--'))
add(schema)
add("CREATE PROCEDURE assert_true(v BOOL, message VARCHAR(128)) BEGIN IF NOT COALESCE(v,FALSE) THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT=message; END IF; END")
for boss in (15740, 15741, 15742):
    add(insert.format(1, 100, boss, 150))
    add(insert.format(1, 100, boss, 999))
    check(select.format(1, 100, boss), 'exact key present')
    check(f'SELECT killed_at=150 FROM aq_war_effort_boss_kill WHERE boss_id={boss}', 'duplicate preserves original time')
add(schema)
check('SELECT COUNT(*)=3 FROM aq_war_effort_boss_kill', 'rerunnable schema preserves history')
for campaign, origin in ((1, 200), (2, 100)):
    for boss in (15740, 15741, 15742):
        check('SELECT ('+select.format(campaign, origin, boss)+')=0', 'fresh epoch and campaign independent')
add(insert.format(1, 200, 15741, 250))
check('SELECT ('+select.format(1, 200, 15742)+')=0', 'Regal death leaves Ashi alive')
check('SELECT COUNT(*)=4 FROM aq_war_effort_boss_kill', 'new epoch preserves old deaths')
check('SELECT ('+select.format(1, 200, 15740)+')=0', 'Regal death leaves Zora alive')
add(insert.format(1, 200, 15740, 260))
check('SELECT ('+select.format(1, 200, 15742)+')=0', 'Regal and Zora deaths leave Ashi alive')
for mask in range(8):
    for index, boss in enumerate((15742, 15741, 15740)):
        if mask & (1 << index): add(insert.format(9, 1000+mask, boss, 2000))
    for index, boss in enumerate((15742, 15741, 15740)):
        check('SELECT ('+select.format(9, 1000+mask, boss)+')='+str(int(bool(mask & (1 << index)))), 'three-boss defeat matrix')
with TemporaryDirectory(prefix='aq-boss-sql-') as directory:
    work = Path(directory)
    marker = work / 'passed.txt'
    add(f"SELECT 'PASS MySQL: actual named-boss insert/read, independent boss/epoch/campaign keys, duplicate timestamps, convergent existing schema' INTO OUTFILE '{marker}'")
    init = work / 'init.sql'
    init.write_text('\n'.join(('DELIMITER $$\n'+s+'$$\nDELIMITER ;') if s.startswith('CREATE PROCEDURE') else s+';' for s in statements)+'\n')
    root = args.mysql_root.resolve()
    env = os.environ.copy()
    env['LD_LIBRARY_PATH'] = str(root/'lib/x86_64-linux-gnu')+':'+env.get('LD_LIBRARY_PATH', '')
    data = work / 'data'
    data.mkdir()
    server = root / ('sbin/mysqld' if (root/'sbin/mysqld').exists() else 'bin/mysqld')
    result = subprocess.run([str(server), '--no-defaults', '--initialize-insecure',
                             '--basedir='+str(root), '--datadir='+str(data), '--init-file='+str(init),
                             '--mysqlx=OFF', '--skip-networking', '--secure-file-priv='+str(work)],
                            env=env, capture_output=True, text=True)
    assert result.returncode == 0 and marker.exists(), result.stderr
    print(marker.read_text().strip())
