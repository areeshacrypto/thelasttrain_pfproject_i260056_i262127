#ifndef STATE_H
#define STATE_H

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

// The game state is kept as plain variables and a bool array inside main().
// Every function receives the pieces it needs through pointers, so there is
// no struct anywhere in the program.

inline bool hasItem(const bool itemTaken[], ItemId id) {
    return itemTaken[id];
}

inline int clueCount(bool clueFour, bool clueTwo, bool clueSeven) {
    int count = 0;
    if (clueFour) ++count;
    if (clueTwo) ++count;
    if (clueSeven) ++count;
    return count;
}

// True when every optional and mandatory objective is complete, including
// the final boarding action itself. Used to award the +100 mission bonus.
inline bool isFullMission(bool clueFour, bool clueTwo, bool clueSeven,
                          const bool itemTaken[], bool cabinetUnlocked,
                          bool passengerRescued, bool catSecured,
                          bool fuseInstalled, bool trainBoarded) {
    return itemTaken[IT_MASTER_KEY] && clueFour && clueTwo && clueSeven &&
           cabinetUnlocked && itemTaken[IT_TRAIN_KEY] && passengerRescued &&
           catSecured && fuseInstalled && itemTaken[IT_LIFT_PASS] && trainBoarded;
}

// Renders the discovered digits, e.g. "4 _ 7". Defined in switches.cpp
// because both the HUD and the collect logic use it.
void formatClueCode(bool clueFour, bool clueTwo, bool clueSeven, char* out, int cap);

#endif  // STATE_H