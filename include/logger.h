#ifndef LOGGER_H
#define LOGGER_H

#include "level.h"
#include "state.h"

namespace logger {

void resetAudit();
void addAudit(const char* text);
int auditCount();
const char* auditLine(int index);

bool saveGameStateToFile(int energy, int time, int score, int room, int turn,
                         int emergencyTrips,
                         bool fuseInstalled, bool cabinetUnlocked, bool passengerRescued,
                         bool catSecured, bool trainBoarded,
                         const bool itemTaken[],
                         const char roomId[][cfg::kIdLen],
                         const char itemCode[][cfg::kItemIdLen],
                         const ItemId itemAs[], int itemCount,
                         const char* path, char* error, int errorCap);

bool loadGameStateFromFile(int* energy, int* time, int* score, int* room, int* turn,
                           int* emergencyTrips,
                           bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
                           bool* catSecured, bool* trainBoarded,
                           bool* clueFour, bool* clueTwo, bool* clueSeven,
                           bool itemTaken[], bool* over, bool* victory,
                           const char roomId[][cfg::kIdLen], int roomCount,
                           const char itemCode[][cfg::kItemIdLen],
                           const char itemName[][cfg::kNameLen],
                           const char itemSymbol[], const ItemId itemAs[], int itemCount,
                           const char* path, char* error, int errorCap);

bool writeGameSummaryReport(int energy, int time, int score, int room, int turn,
                            bool fuseInstalled, bool cabinetUnlocked, bool passengerRescued,
                            bool catSecured, bool trainBoarded, bool victory,
                            bool clueFour, bool clueTwo, bool clueSeven,
                            const bool itemTaken[],
                            const char roomId[][cfg::kIdLen],
                            const char roomName[][cfg::kNameLen],
                            const char* result, const char* path,
                            char* error, int errorCap);

}  // namespace logger

#endif  // LOGGER_H