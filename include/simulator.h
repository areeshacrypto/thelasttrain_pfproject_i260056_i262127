#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <cstdio>

#include "level.h"
#include "state.h"

// Runs the command loop until victory, defeat, quit, or EOF.
// echoCommands = true replays script input into stdout so a transcript
// shows exactly which commands produced the output.
// Returns 0 for victory, 1 for defeat, 2 for incomplete/error.
namespace simulator {

int runSimulation(const BuildingMap* map, GameState* state, std::FILE* input,
                  bool echoCommands);

}  // namespace simulator

#endif  // SIMULATOR_H
