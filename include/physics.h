#ifndef PHYSICS_H
#define PHYSICS_H

#include "level.h"

// The "physics" layer owns the resource economy: what an action costs, how
// resources are clamped, and when the mission is lost. A cost is returned
// through two int pointers instead of a struct.

void costForMove(LinkType type, int* costEnergy, int* costTime);
void costInspect(int* costEnergy, int* costTime);
void costPickup(int* costEnergy, int* costTime);
void costUse(int* costEnergy, int* costTime);

void applyCost(int* energy, int* time, int costEnergy, int costTime);
void addEnergy(int* energy, int delta);
void addTime(int* time, int delta);

// Clamps energy/time into [0, 100] without checking failure.
void clampResources(int* energy, int* time);

// Clamps, then marks the run as lost if energy or time reached zero.
// Returns true when the game is over. Never overwrites a victory.
bool resolveDefeat(int* energy, int* time, bool* over, bool* victory);

#endif  // PHYSICS_H