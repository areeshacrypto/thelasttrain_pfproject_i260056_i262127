#ifndef SWITCHES_H
#define SWITCHES_H

#include "level.h"
#include "state.h"

// Interaction rules. Every function returns false when the action is
// rejected, and a rejected action must not touch resources or state.
namespace switches {

bool tryTraverse(GameState* state, const BuildingMap* map, int targetRoom,
                 char* out, int cap);

bool tryCollect(GameState* state, const BuildingMap* map, const char* token,
                char* out, int cap);

bool tryUse(GameState* state, const BuildingMap* map, ItemId target,
            int argument, char* out, int cap);

}  // namespace switches

#endif  // SWITCHES_H
