#ifndef SWITCHES_H
#define SWITCHES_H

#include "level.h"
#include "state.h"

// Interaction rules. Every function returns false when the action is
// rejected, and a rejected action must not touch resources or state.
// The state is passed piece by piece through int*/bool* pointers; the map is
// passed as the read-only parallel arrays it needs.
namespace switches {

bool tryTraverse(int* energy, int* time, int* room, int* emergencyTrips, int* turn,
                 const bool itemTaken[], bool fuseInstalled,
                 bool* over, bool* victory,
                 const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                 const int roomFloor[], const int linkA[], const int linkB[],
                 const LinkType linkType[], int linkCount, int roomCount,
                 int targetRoom, char* out, int cap);

bool tryCollect(int* energy, int* time, int* score, int* room, int* turn,
                int emergencyTrips,
                bool itemTaken[], bool* clueFour, bool* clueTwo, bool* clueSeven,
                bool cabinetUnlocked, bool* over, bool* victory,
                const char roomId[][cfg::kIdLen],
                const char itemCode[][cfg::kItemIdLen], const char itemName[][cfg::kNameLen],
                const char itemSymbol[], const int itemRoom[], const ItemClass itemClass[],
                const ItemId itemAs[], const int itemValue[], int itemCount,
                const char* token, char* out, int cap);

bool tryUse(int* energy, int* time, int* score, int* room, int* turn,
            bool itemTaken[], bool* clueFour, bool* clueTwo, bool* clueSeven,
            bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
            bool* catSecured, bool* trainBoarded, bool* over, bool* victory,
            const char roomId[][cfg::kIdLen], int roomCount,
            const char itemName[][cfg::kNameLen], const int itemRoom[],
            const ItemId itemAs[], int itemCount,
            ItemId target, int argument, char* out, int cap);

}  // namespace switches

#endif  // SWITCHES_H