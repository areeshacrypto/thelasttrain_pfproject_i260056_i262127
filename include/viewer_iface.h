#ifndef VIEWER_IFACE_H
#define VIEWER_IFACE_H

#include "level.h"
#include "state.h"

// Presentation only. The core never prints game rules on its own, which
// keeps the simulation and the console UI separable.
namespace viewer {

void printBanner();

void printStatus(int energy, int time, int score, int turn, int room,
                 const bool itemTaken[], bool fuseInstalled,
                 bool passengerRescued, bool catSecured,
                 bool clueFour, bool clueTwo, bool clueSeven, bool cabinetUnlocked,
                 const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                 const int roomFloor[], const int linkA[], const int linkB[],
                 const LinkType linkType[], int linkCount,
                 const char itemName[][cfg::kNameLen], const char itemSymbol[],
                 const int itemRoom[], const ItemClass itemClass[],
                 const ItemId itemAs[], int itemCount);

void printMapOverview(const int grid[][cfg::kRoomsPerFloor],
                      const char roomId[][cfg::kIdLen], int room);

void printHelp();

void printResult(int energy, int time, int score, int turn, bool victory);

}  // namespace viewer

#endif  // VIEWER_IFACE_H