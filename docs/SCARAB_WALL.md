# Scarab Wall: closed versus absent

This document describes the original closed-versus-absent wall milestone. The
same controller now also presents successful gong openings; see
[SCARAB_GONG.md](SCARAB_GONG.md). The phase-command and startup tests below still
apply. Collection behavior and quest 8519 remain unchanged.

## Source audit and placement

Reference core: AzerothCore commit `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`.
`src/server/scripts/Kalimdor/zone_silithus.cpp`, quest 8519 (A Pawn on the Eternal
Board), steps 37, 38 and 41, summons these exact objects at the same position:

| Entry | Object | Map | X | Y | Z | Orientation | Quaternion x/y/z/w |
|---|---|---:|---:|---:|---:|---:|---|
| 176146 | Gate of Ahn'Qiraj | 1 | -8130 | 1525 | 17.5 | 0 | 0 / 0 / 0 / 1 |
| 176147 | Ahn'Qiraj Gate Roots | 1 | -8130 | 1525 | 17.5 | 0 | 0 / 0 / 0 / 1 |
| 176148 | Ahn'Qiraj Gate Runes | 1 | -8130 | 1525 | 17.5 | 0 | 0 / 0 / 0 / 1 |

The script passes an all-zero quaternion. `GameObject::SetWorldRotation` converts
that to a rotation derived from orientation, then normalizes it. Orientation zero
therefore gives the identity quaternion above. SQL stores the normalized value;
no placement values were guessed. The reference summons use animprogress 100 and
GO_STATE_READY. Stock templates are display IDs 4132/4133/4134, types door/button/
button. The checked-out stock gameobject table has no persistent placements for
these entries. The quest scene constructs the historical seal; its temporary
objects are reference material and are not hijacked by this module.

## Installation and ownership

With worldserver stopped, apply
`data/sql/db-world/base/002_aq_scarab_wall.sql` to the world database. Apply this
new file to existing installs too, then rebuild/install the module and restart.
No additional character migration is introduced by this milestone.

The SQL creates only spawns 9100146/9100147/9100148 with the corresponding entries,
on map 1, spawn mask 1, phase mask 1, and per-spawn ScriptName
`go_aq_war_effort_wall`. It does not modify shared gameobject templates. On rerun,
only already-owned rows can be reconciled to canonical placement. A conflicting
reserved GUID, another wall within 10 horizontal units of the canonical location,
or an event/pool association causes an error and rolls back instead of overwriting
or duplicating existing data. Resolve such collisions explicitly; do not delete
unrelated rows simply to bypass the guard. Execute SQL serially.

Runtime ownership requires matching map, reserved spawn ID, entry and script ID.
Other objects sharing these entries, including quest 8519's temporary summons,
are untouched. No database deletes, spawn toggles or state writes occur at runtime.

## Reconciliation

When the module is enabled and campaign data is available:

- WAR_EFFORT and READY: GO_STATE_READY, phase mask 1, collision enabled.
- TEN_HOUR_WAR and OPEN: phase mask 0 and collision disabled; the model is absent
  rather than playing an opening animation. The closed pose is retained internally.
- DISABLED: no wall reads/writes/grid loading initiated by the reconciler and no
  player interaction interception. This is not an implicit open/close command.

Startup loads the canonical grid and applies persisted phase after successful
campaign loading. Successful phase transitions and same-phase administrative
commands reconcile immediately. Ordinary contributions do not repeatedly load
or log the wall unless they change phase. AddWorld hooks handle later grid loads,
respawns and reloads; Update hooks keep only the three owned objects consistent.
No GameObject pointers are retained across grid unloads. Runtime visibility and
collision changes are never saved as progression; the campaign remains the sole
source of truth. Normal right-click interaction is blocked while managed so stock
door/button use cannot bypass the campaign phase.

When disabled/unavailable, the module leaves current loaded object state alone.
SQL's baseline is closed: a later grid load/restart while DISABLED (or with the
module config off) can load that baseline, without active phase management. The
module does not promise to freeze an unmanaged state across grid unloads.

`GameObject::SetGoState` and `EnableCollision` in the inspected core control the
stock collision model. Explicit collision disabling accompanies visibility removal;
this avoids an invisible residual wall. No extra barrier was added. Correct client
models and extracted server collision data are still required; physical movement
needs the live test below. A running 8519 scene can independently summon its own
wall, intentionally outside this module's ownership.

## Test on a staging realm

1. Stop worldserver, back up world DB, apply the SQL twice. Confirm exactly the
   three reserved rows, with the positions/rotations above and correct script name.
   Verify unrelated rows remain unchanged. Start the rebuilt module with valid
   existing config/data and AQWarEffort.Enable=1.
2. Use an administrator to run `.aqwareffort phase effort`. Check the log says all
   three owned objects were reconciled closed. Visit the stock gate from outside
   using an ordinary player in phase 1 (no GM flight/noclip or active 8519 scene).
   Confirm roots, runes and gate appear and walking through the gate is blocked.
   Right-click must not open the wall.
3. Run `.aqwareffort phase ready`: still closed. Repeat the command: no duplicates.
4. Run `.aqwareffort phase war`, then `open`: all three disappear immediately;
   walk across the former wall position and confirm there is no residual collision.
   Confirm no ceremony, sounds or battle content starts.
5. Return to `ready` or `effort`: the three stock objects return and block passage.
6. Restart separately in READY and OPEN, including a restart with nobody in
   Silithus. Verify the expected wall before and after players arrive. Leave the
   grid long enough to unload, return, and verify the phase is reapplied.
7. Switch to DISABLED from both a closed and an absent state. Confirm the module
   does not actively switch the wall; test stock baseline on reload as described.
8. Verify scores, contribution quantities, the existing event-22 behavior, and
   stored campaign timestamps are unchanged except for normal phase transitions.
   Confirm quest 8519 temporary objects and unrelated gameobjects are not modified.

Build validation compiles the module against the inspected core. A mock-core
harness executes the actual wall code for phases, ownership, reload, collision,
interaction and idempotence. These checks do not replace the live movement test.

## Gong milestone integration

The current controller also owns the live opening ceremony; see
[SCARAB_GONG.md](SCARAB_GONG.md). Only a successful, durably accepted authorized
gong reward enables the transient animation stage. Administrative phase commands
and startup reconciliation continue to use the immediate closed/absent behavior
described above. No second controller or replacement wall spawns are introduced.
