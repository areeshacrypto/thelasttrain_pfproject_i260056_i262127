#ifndef LOGGER_H
#define LOGGER_H

#include "level.h"
#include "state.h"

namespace logger {

void resetAudit();
void addAudit(const char* text);
int auditCount();
const char* auditLine(int index);

bool saveGameStateToFile(const GameState* state, const BuildingMap* map,
                         const char* path, char* error, int errorCap);

bool loadGameStateFromFile(GameState* state, const BuildingMap* map,
                           const char* path, char* error, int errorCap);

bool writeGameSummaryReport(const GameState* state, const BuildingMap* map,
                            const char* result, const char* path,
                            char* error, int errorCap);

}  // namespace logger

#endif