#ifndef VIEWER_IFACE_H
#define VIEWER_IFACE_H

#include "level.h"
#include "state.h"

// Presentation only. The core never prints game rules on its own, which
// keeps the simulation and the console UI separable.
namespace viewer {

void printBanner();
void printStatus(const BuildingMap* map, const GameState* state);
void printMapOverview(const BuildingMap* map, const GameState* state);
void printHelp();
void printResult(const BuildingMap* map, const GameState* state);

}  // namespace viewer

#endif  // VIEWER_IFACE_H
