#ifndef PHYSICS_H
#define PHYSICS_H

#include "level.h"
#include "state.h"

// The "physics" layer owns the resource economy: what an action costs, how
// resources are clamped, and when the mission is lost.
struct Cost {
    int energy;
    int time;
};

Cost costForMove(LinkType type);
Cost costInspect();
Cost costPickup();
Cost costUse();

void applyCost(GameState* state, const Cost& cost);
void addEnergy(GameState* state, int delta);
void addTime(GameState* state, int delta);

// Clamps energy/time into [0, 100] without checking failure.
void clampResources(GameState* state);

// Clamps, then marks the run as lost if energy or time reached zero.
// Returns true when the game is over. Never overwrites a victory.
bool resolveDefeat(GameState* state);

#endif  // PHYSICS_H
