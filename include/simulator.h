#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <cstdio>

#include "level.h"
#include "state.h"

// Runs the command loop until victory, defeat, quit, or EOF.
// echoCommands = true replays script input into stdout so a transcript
// shows exactly which commands produced the output.
// Returns 0 for victory, 1 for defeat, 2 for incomplete/error.
// The map and state arrive as the parallel arrays and scalar pointers that
// main() owns; there is no struct to bundle them.
namespace simulator {

int runSimulation(int* energy, int* time, int* score, int* room,
                  int* emergencyTrips, int* turn,
                  bool itemTaken[],
                  bool* clueFour, bool* clueTwo, bool* clueSeven,
                  bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
                  bool* catSecured, bool* trainBoarded, bool* over, bool* victory,
                  const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                  const int roomFloor[],
                  const int linkA[], const int linkB[], const LinkType linkType[],
                  int linkCount, int roomCount,
                  const char itemCode[][cfg::kItemIdLen], const char itemSymbol[],
                  const char itemName[][cfg::kNameLen], const int itemRoom[],
                  const ItemClass itemClass[], const ItemId itemAs[],
                  const int itemValue[], int itemCount,
                  const int grid[][cfg::kRoomsPerFloor],
                  std::FILE* input, bool echoCommands);

}  // namespace simulator

#endif  // SIMULATOR_H