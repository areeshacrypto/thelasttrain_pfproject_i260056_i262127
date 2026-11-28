# The Last Train

A deterministic console rescue game in C++17. You have five floors, twenty
rooms, one injured passenger, one stranded cat, and a train that will not leave
without you.

## Build

```sh
g++ -std=c++17 -Wall -Wextra -Iinclude src/core/*.cpp -o last_train
```

or

```sh
cmake -S . -B build && cmake --build build
```

The build must finish with **no warnings**.

## Run

```sh
./last_train                     # interactive session (reads stdin)
./last_train data/test_win.txt   # scripted session, echoes commands
./last_train data/test_lose.txt  # deterministic defeat
./last_train --load              # resume from ./out/savegame.txt
```

All paths are relative, so run from the repository root. `./out/` must exist
(it is committed with the sample output).

Exit codes: `0` victory, `1` defeat, `2` incomplete/error.

## Determinism check

```sh
./last_train data/test_win.txt && cp out/game_summary.txt /tmp/a.txt
./last_train data/test_win.txt && diff /tmp/a.txt out/game_summary.txt
```

There is no randomness, no clock input and no unordered iteration, so the
two runs are byte-identical. `data/test_win.txt` scores **520/520** and finishes
with 16 energy and 7 time.

## Comparing with another build

`data/test_compare.txt` is a short acceptance script meant to be run by any
team on any copy of the game. Every probe targets one rule, including the ones
that must be *refused without cost*:

```sh
./last_train data/test_compare.txt
```

Expected signature on a correct build:

* `data/test_win.txt`: exit 0, VICTORY, energy 16, time 7, turn 64, score 520/520
* `data/test_lose.txt`: exit 1, DEFEAT, energy 0, time 32, turn 34, score 0/520
* `data/test_compare.txt`: exit 2, INCOMPLETE, energy 77, time 86, turn 12, score 20/520

To compare, exchange three things: the console transcript, `out/game_summary.txt`,
and the process exit code (`echo $?` on bash, `echo %errorlevel%` on cmd). Any
difference in one of those numbers above points at the rule that diverges.

If you built with the winlibs GCC 16 download, add `-static` to the build line;
otherwise the exe may load an older `libstdc++-6.dll` from `PATH` and crash.

## Commands

* `LOOK` / `STATUS`: room, resources, inventory, clues, exits
* `MAP`: 5x4 building overview
* `MOVE <room>`: move along an adjacent link (`MOVE F3-R2`)
* `INSPECT`: search the room (costs 1 energy, 1 time)
* `COLLECT <item>`: id, name or symbol (costs 1 energy, 1 time)
* `USE FUSE`: install the Fuse at the Control Panel
* `USE MEDKIT`: treat the passenger in F4-R1
* `USE CAT_CARRIER`: secure the cat in F3-R3
* `UNLOCK CABINET 427`: open the cabinet in F5-R1
* `BOARD`: board the train at F5-R2
* `SAVE` / `LOAD`: `./out/savegame.txt`
* `QUIT`: end the session

Movement costs: a door or staircase takes 2 energy and 1 time, a window takes
3 energy and 2 time, a lift takes 4 energy and 3 time. Rejected actions (wrong
room, missing item, locked door, no lift trip left) cost nothing.

## Rules that are easy to get wrong

* **Lift 1** joins any of `F1-R2`/`F1-R3` to any of `F2-R2`/`F2-R3` (4 links).
* **Lift 2** joins any pair of `F2-R4`, `F4-R4`, `F5-R4`. Before repair it needs
  the Emergency Access Card and consumes one of 3 trips; after the Fuse is
  installed it needs the Lift Pass and is unlimited.
* **Clue digits** are picked up as items (1E/1T) and grant +10 each.
* **Cat / passenger**: the carrier / medical kit must be in the inventory *and*
  used on site.
* **Boarding** at F5-R2 needs the Train Key, the installed Fuse, the rescued
  passenger and the secured cat. The +100 full mission bonus additionally needs
  the Master Key, all three clues, the open cabinet and the Lift Pass.

## Score (max 520)

Master Key 10, three clues 10 each, cabinet 30, Train Key 10, passenger 100,
cat 100, Fuse 20, Lift Pass 20, boarding 100, full mission 100.

## Balance note (important)

The handout starts the player with **50 time**. A breadth first search over the
exact graph with the handout's costs proves that the cheapest full clear route
needs **60 time**, so 50 makes victory impossible (the best finish is a defeat
on the way back from F3-R3). This repository therefore starts at **100 time**
in `include/config.h` and `data/initial-state.txt`. Every other number is
exactly as specified; the Observation Token (+15 time) still matters if you
lower the start value. To restore the handout value, set `kStartTime` and the
`TIME` line in `data/initial-state.txt` to 50 and accept that no run can win.

## File formats

`data/building.txt`: `VERSION`, `ROOMS n`, `ROOM id|name`, `LINKS n`,
`LINK a|b|TYPE` (`DOOR WINDOW STAIR LOCKED LIFT1 LIFT2 WALL`), `ITEMS n`,
`ITEM id|symbol|name|room|class|value`, `GRID` (5 rows x 4 rooms, coordinates),
`END`. Loaded once; never written.

`data/initial-state.txt` - same schema as a savegame.

`out/savegame.txt`: `VERSION`, `TURN`, `ROOM`, `ENERGY`, `TIME`, `SCORE`,
`TRIPS`, `FUSE_INSTALLED`, `CABINET_OPEN`, `PASSENGER_RESCUED`, `CAT_SECURED`,
`TRAIN_BOARDED`, `TAKEN id,id,...` (or `TAKEN -`).

`out/game_summary.txt`: result, resources, score, objective checklist and the
full audit trail; rewritten on every session end (VICTORY / DEFEAT /
INCOMPLETE).

## Architecture

* `include/config.h`: every balance constant and I/O path
* `include/state.h`: item enum, objective helpers (`hasItem`, `clueCount`, `isFullMission`)
* `include/level.h` + `level.cpp`: map model as parallel arrays, strict loader, `loadBuildingMap()`, `findRoom()`, `findLink()`, `validateStartingRoom()`, `buildExitList()`
* `include/physics.h` + `physics.cpp`: cost table, clamping, `resolveDefeat()`
* `include/switches.h` + `switches.cpp`: `tryTraverse()`, `tryCollect()`, `tryUse()` (locked door, lifts, cabinet, fuse, rescue, cat, boarding)
* `include/logger.h` + `logger.cpp`: audit trail, `saveGameStateToFile()`, `loadGameStateFromFile()`, `writeGameSummaryReport()`
* `include/viewer_iface.h` + `viewer_iface_core.cpp`: HUD, map, help, result
* `include/simulator.h` + `simulator.cpp`: command loop, `handleMoveCommand()`, `handleInspectCommand()`, `handleCollectCommand()`, `handleUseCommand()`
* `main_core.cpp`: argument parsing, state bootstrap, shutdown

Data flow: `main_core` owns the map arrays and the state variables, then
`simulator` parses one command, `switches` validates and mutates the state
through `physics`, `logger` records the action, `viewer` renders the HUD. On
exit `logger` writes the summary.

## CLO mapping

* **CLO-1**: branching command dispatch, adjacency checks, prerequisite ladders
  (key -> door, clues -> cabinet, fuse+pass -> lift 2, objectives -> boarding).
* **CLO-2**: modular free functions, 1D/2D arrays (`roomId[]`, `linkA[]`,
  `itemTaken[]`), `char` buffers, pointers passed explicitly, no classes and
  no structs.
* **CLO-3**: deterministic output, stable savegame/summary, state restored
  across sessions with `--load`.

