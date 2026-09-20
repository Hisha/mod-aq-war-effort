from pathlib import Path
import re,subprocess,os,json,argparse
from tempfile import TemporaryDirectory
parser=argparse.ArgumentParser(description='Isolated MySQL 8 initialization tests; never connects to a running database.')
parser.add_argument('--core',type=Path,required=True)
parser.add_argument('--mysql-root',type=Path,required=True,help='MySQL installation prefix containing sbin/mysqld or bin/mysqld')
args=parser.parse_args()
temporary=TemporaryDirectory(prefix='aq-gong-sql-')
w=Path(temporary.name)
p=Path(__file__).resolve().parents[1]
core=args.core.resolve()/'data/sql/base/db_world'

statements=['CREATE DATABASE aq_gong_test','USE aq_gong_test']
def add(s):statements.append(s.strip().rstrip(';'))
for t in ['gameobject','gameobject_template','gameobject_queststarter','gameobject_questender','creature_queststarter','creature_questender','game_event_gameobject','pool_gameobject']:
 add(re.search(r'CREATE TABLE .*?;\n',(core/(t+'.sql')).read_text(),re.S)[0])
for t,ids in [('gameobject_template',[180717,180718]),('gameobject',[49451])]:
 for l in (core/(t+'.sql')).read_text().splitlines():
  if any(l.startswith('('+str(i)+',') for i in ids):add('INSERT INTO '+t+' VALUES '+l.rstrip(',;')+';')
add("INSERT INTO gameobject (guid,id,map,position_x,position_y) VALUES (123,42,1,1,2)")
# mysql --init-file accepts one complete statement per line (no client DELIMITER directives).
s=(p/'data/sql/db-world/base/003_aq_scarab_gong.sql').read_text()
proc=re.search(r'CREATE PROCEDURE .*?END\$\$',s,re.S)[0][:-2]
proc='\n'.join(l for l in proc.splitlines() if not l.strip().startswith('--'))
add(proc)
add("CREATE PROCEDURE assert_true(v BOOL, message VARCHAR(128)) BEGIN IF NOT COALESCE(v, FALSE) THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT=message; END IF; END")
def check(expr,msg):add(f"CALL assert_true(({expr}), '{msg}')")
add('CALL aq_war_effort_restore_gong()');add('CALL aq_war_effort_restore_gong()')
check('SELECT COUNT(*)=1 FROM gameobject WHERE id=180717','one usable gong')
check('SELECT COUNT(*)=0 FROM gameobject WHERE guid=49451','decoration replaced')
check('SELECT COUNT(*)=1 FROM gameobject WHERE guid=123 AND id=42 AND position_x=1','unrelated spawn intact')
check('SELECT type=5 FROM gameobject_template WHERE entry=180718','decoration template unchanged')
check('SELECT COUNT(*)=1 FROM gameobject_queststarter WHERE id=180717 AND quest=8743','quest starter convergent')
check('SELECT COUNT(*)=1 FROM gameobject_questender WHERE id=180717 AND quest=8743','quest ender convergent')
add('UPDATE gameobject SET position_x=0 WHERE guid=9100717');add('CALL aq_war_effort_restore_gong()')
check('SELECT ABS(position_x+8069.05)<0.01 FROM gameobject WHERE guid=9100717','owned position converges')
add("CREATE PROCEDURE expect_conflict() BEGIN DECLARE failed BOOL DEFAULT FALSE; BEGIN DECLARE CONTINUE HANDLER FOR SQLSTATE '45000' SET failed=TRUE; CALL aq_war_effort_restore_gong(); END; CALL assert_true(failed,'expected SQL conflict'); END")
add('UPDATE gameobject SET id=42 WHERE guid=9100717');add('CALL expect_conflict()')
check('SELECT id=42 FROM gameobject WHERE guid=9100717','collision not overwritten');add('UPDATE gameobject SET id=180717 WHERE guid=9100717')
add('UPDATE gameobject_template SET type=2 WHERE entry=180718');add('CALL expect_conflict()');add('UPDATE gameobject_template SET type=5 WHERE entry=180718')
add('INSERT INTO game_event_gameobject VALUES (22,9100717)');add('CALL expect_conflict()');add('DELETE FROM game_event_gameobject WHERE guid=9100717')
add('INSERT INTO gameobject_questender VALUES (180718,8743)');add('CALL expect_conflict()');add('DELETE FROM gameobject_questender WHERE id=180718')
add("INSERT INTO gameobject (guid,id,map) VALUES (49451,42,1)");add('CALL expect_conflict()')
check('SELECT id=42 FROM gameobject WHERE guid=49451','custom decoration retained');add('DELETE FROM gameobject WHERE guid=49451');add('CALL aq_war_effort_restore_gong()')
# Character migration and the EXACT acceptance statement extracted from the production C++.
for fn in ['001_aq_war_effort.sql','002_aq_scarab_gong.sql']:
 text=(p/'data/sql/db-characters/base'/fn).read_text();text='\n'.join(l for l in text.splitlines() if not l.startswith('--'))
 for st in text.split(';'):
  if st.strip():add(st)
add('CREATE TABLE character_queststatus_rewarded (guid INT UNSIGNED,quest INT UNSIGNED,PRIMARY KEY(guid,quest)) ENGINE=InnoDB')
add('INSERT INTO aq_war_effort_campaign VALUES (1,2,100,77,88)');add('INSERT INTO aq_war_effort (id,faction,bandages01) VALUES (1,0,12345),(1,1,56789)')
add('INSERT INTO aq_war_effort_gong VALUES (1,10,100,101,0)')
cpp=(p/'src/mod_aq_war_effort.cpp').read_text();block=cpp[cpp.index('"UPDATE aq_war_effort_campaign c JOIN'):];block=block[:block.index('QuestBangGong, uint32')]
query=''.join(json.loads(x) for x in re.findall(r'"(?:[^"\\]|\\.)*"',block)).format(8743,3,999,999,1,2)
add(query);check('SELECT phase=2 FROM aq_war_effort_campaign WHERE id=1','intent alone never opens')
add('INSERT INTO character_queststatus_rewarded VALUES (10,8743)');add(query)
check('SELECT phase=3 AND gong_rung_at=phase_started_at AND opened_at=88 FROM aq_war_effort_campaign WHERE id=1','atomic phase and timestamps')
check('SELECT accepted=1 FROM aq_war_effort_gong WHERE id=1','journal accepted atomically')
check('SELECT SUM(bandages01)=69134 FROM aq_war_effort WHERE id=1','materials preserved')
add('UPDATE aq_war_effort_campaign SET gong_rung_at=123,phase_started_at=123 WHERE id=1');add(query)
check('SELECT gong_rung_at=123 FROM aq_war_effort_campaign WHERE id=1','accepted attempt idempotent')
add('UPDATE aq_war_effort_campaign SET phase=2,phase_started_at=200 WHERE id=1');add('UPDATE aq_war_effort_gong SET accepted=0 WHERE id=1');add(query)
check('SELECT phase=2 FROM aq_war_effort_campaign WHERE id=1','wrong READY epoch does not open')
marker=w/'sql-passed.txt';add(f"SELECT 'PASS MySQL: convergent installation, conflict rollback, exact atomic acceptance SQL, durable reward requirement, idempotence, epoch guard, preserved counters' INTO OUTFILE '{marker}'")
init=w/'mysql-test-init.sql';init.write_text('\n'.join(('DELIMITER $$\n'+s+'$$\nDELIMITER ;') if s.startswith('CREATE PROCEDURE') else s+';' for s in statements)+'\n')
root=args.mysql_root.resolve();env=os.environ.copy();env['LD_LIBRARY_PATH']=str(root/'lib/x86_64-linux-gnu')+':'+env.get('LD_LIBRARY_PATH','');data=w/'mysql-test-data';
if data.exists():
 import shutil
 shutil.rmtree(data)
data.mkdir()
with (w/'mysql-test.log').open('w') as log:
 r=subprocess.run([str(root/('sbin/mysqld' if (root/'sbin/mysqld').exists() else 'bin/mysqld')),'--no-defaults','--initialize-insecure','--basedir='+str(root),'--datadir='+str(data),'--init-file='+str(init),'--mysqlx=OFF','--skip-networking','--secure-file-priv='+str(w)],env=env,stdout=log,stderr=log)
assert r.returncode==0,(w/'mysql-test.log').read_text()
assert marker.exists(),(w/'mysql-test.log').read_text()
print(marker.read_text())
