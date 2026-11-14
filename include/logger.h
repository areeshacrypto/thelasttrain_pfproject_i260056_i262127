#ifndef LOGGER_H
#define LOGGER_H

#include "level.h"
#include "state.h"

// Persistence and the session audit trail.
namespace logger {

void resetAudit();
void addAudit(const char* format, ...);
int auditCount();
const char* auditLine(int index);

// Snapshot serialisation (also used to read data/initial-state.txt, which
// shares the savegame schema).
bool saveGameStateToFile(const GameState* state, const BuildingMap* map,
                         const char* path, char* error, int errorCap);

bool loadGameStateFromFile(GameState* state, const BuildingMap* map,
                           const char* path, char* error, int errorCap);

// Written once when the session ends: VICTORY, DEFEAT, or INCOMPLETE.
bool writeGameSummaryReport(const GameState* state, const BuildingMap* map,
                            const char* result, const char* path,
                            char* error, int errorCap);

}  // namespace logger

#endif  // LOGGER_H
