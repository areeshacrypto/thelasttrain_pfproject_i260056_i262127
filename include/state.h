#ifndef STATE_H
#define STATE_H

#include <cstdio>

#include "config.h"

// Stable item identifiers. The textual ids in data/building.txt are mapped
// onto these values by level.cpp, so the simulation logic never matches on
// strings.
enum ItemId {
    IT_MASTER_KEY = 0,
    IT_ENERGY_PACK_A,
    IT_ENERGY_PACK_B,
    IT_TOOLKIT,
    IT_CLUE_FOUR,
    IT_FUSE,
    IT_CAT,
    IT_PANEL,
    IT_LIFT_PASS,
    IT_PASSENGER,
    IT_MEDKIT,
    IT_ACCESS_CARD,
    IT_CLUE_TWO,
    IT_CLUE_SEVEN,
    IT_CABINET,
    IT_TRAIN_KEY,
    IT_CAT_CARRIER,
    IT_OBSERVATION_TOKEN,
    IT_COUNT
};

// The complete serialisable game state. POD only: no pointers, no classes,
// so a snapshot is exactly what gets written to out/savegame.txt.
struct GameState {
    int energy;
    int time;
    int score;
    int room;             // index into BuildingMap::rooms
    int emergencyTrips;   // remaining Lift 2 trips before repair
    int turn;             // accepted actions, used by the audit log

    bool clueFour;
    bool clueTwo;
    bool clueSeven;

    bool fuseInstalled;
    bool cabinetUnlocked;
    bool passengerRescued;
    bool catSecured;
    bool trainBoarded;

    bool itemTaken[IT_COUNT];

    bool over;
    bool victory;
};

inline void initGameState(GameState* state, int startRoom) {
    *state = GameState{};
    state->energy = cfg::kStartEnergy;
    state->time = cfg::kStartTime;
    state->score = 0;
    state->room = startRoom;
    state->emergencyTrips = cfg::kStartEmergencyTrips;
    state->turn = 0;
}

inline bool hasItem(const GameState* state, ItemId id) {
    return state->itemTaken[id];
}

inline int clueCount(const GameState* state) {
    int count = 0;
    if (state->clueFour) ++count;
    if (state->clueTwo) ++count;
    if (state->clueSeven) ++count;
    return count;
}

// True when every optional and mandatory objective is complete except the
// final boarding action itself. Used to award the +100 mission bonus.
inline bool isFullMission(const GameState* state) {
    return state->itemTaken[IT_MASTER_KEY] && state->clueFour &&
           state->clueTwo && state->clueSeven && state->cabinetUnlocked &&
           state->itemTaken[IT_TRAIN_KEY] && state->passengerRescued &&
           state->catSecured && state->fuseInstalled &&
           state->itemTaken[IT_LIFT_PASS] && state->trainBoarded;
}

// Renders the discovered digits, e.g. "4 _ 7". Declared here and defined in
// switches.cpp because it is shared by the HUD and the collect logic.
void formatClueCode(const GameState* state, char* out, int cap);

#endif  // STATE_H
