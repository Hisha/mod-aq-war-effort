# Gong milestone validation

Prepared against module commit `905481fc63d13282533b0b1687ccd2403cf5ef7c`
and AzerothCore commit `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`.
The original module checkout was left unchanged. No commits were created.

Passed:

- C++20 syntax/API checks against the inspected core's real headers for
  AQScarabGong.cpp, AQScarabWall.cpp, mod_aq_war_effort.cpp and the loader, with
  `-Wall -Wextra -Werror`. No full worldserver build/link was performed.
- AzerothCore C++ style checks and whitespace/diff checks.
- Executable harness using the actual gong and wall source with a mock core/DB:
  all phase guards, exact ownership, preserved packet cursor, concurrent
  reservation, stock rejection, delayed asynchronous reward persistence, durable
  recovery without replay, disabled recovery, failed/ambiguous writes, wrong READY
  epoch, all ceremony stages and sounds, grid reload reconciliation, unrelated
  cinematic objects, preserved contribution/open timestamps and idempotence.
- Isolated MySQL 8.4 initialization tests using stock table schemas and gong rows:
  fresh install/reapplication, dedicated placement convergence, preservation of
  unrelated rows/templates, reserved GUID conflict, legacy conversion/quest-giver
  conflict, event ownership and customized decorative spawn rejection. The exact
  production acceptance SQL was tested for required reward durability, atomic
  journal/phase acceptance, timestamp/data preservation, idempotence and epoch guard.
  Tests used a disposable database directory, not a running game database.
- Existing collection regression harness adapted to the changed manager, with gong
  initialization stubbed for isolation: all 60 material reward IDs, quantities,
  event-22 synchronization, persistence, configuration/quest validation, overflow,
  disabled state and phase authority remained passing.

Reproducible tests shipped in this patch (run from module root):

```sh
python3 tests/test_scarab_gong.py
python3 tests/test_scarab_gong_sql.py \
  --core /path/to/azerothcore-wotlk \
  --mysql-root /path/to/mysql-installation-prefix
```

The first requires Python 3 and g++ with C++20/pthreads. It compiles the actual
module source against mocks; it is not a replacement for compiling against your
core. The second requires MySQL 8 with its data/share files and creates an isolated
temporary initialized data directory, without listening for network connections.
Both remove their temporary artifacts on exit. They do not connect to or change
production databases. The pre-existing collection harness was an external
validation fixture and is not included in this patch.

Not performed: live WoW client testing, full worldserver linkage, real network
packet tests, real character inventory/reputation/achievement validation, debugger
crash injection on a worldserver, or production deployment. Mock sound assertions
verify server call order, not client acoustics/animations. Follow SCARAB_GONG.md
for the required live tests; the four-second stage timing remains provisional.
