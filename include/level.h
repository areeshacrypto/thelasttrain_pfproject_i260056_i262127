#ifndef LEVEL_H
#define LEVEL_H

#include "config.h"
#include "state.h"

enum LinkType {
    LINK_NONE = 0,
    LINK_DOOR,
    LINK_WINDOW,
    LINK_STAIR,
    LINK_LOCKED,
    LINK_LIFT1,
    LINK_LIFT2,
    LINK_WALL
};

enum ItemClass {
    CLASS_KEY = 0,
    CLASS_ENERGY,
    CLASS_TIME,
    CLASS_CLUE,
    CLASS_QUEST,
    CLASS_ENTITY,
    CLASS_FIXED
};

// The whole building lives in parallel arrays owned by main():
//   rooms  -> roomId, roomName, roomFloor, roomSlot, roomGridRow, roomGridCol
//   links  -> linkA, linkB, linkType
//   items  -> itemCode, itemSymbol, itemName, itemRoom, itemClass, itemAs, itemValue
//   grid   -> grid[floor][column]
// Every function receives the slices it needs as pointers.

// Reads ./data/building.txt into the arrays. Returns false and fills error[]
// on any problem; the counts are reset before parsing so a failed load never
// leaves half data behind.
bool loadBuildingMap(const char* path,
                     char roomId[][cfg::kIdLen], char roomName[][cfg::kNameLen],
                     int roomFloor[], int roomSlot[], int roomGridRow[], int roomGridCol[],
                     int* roomCount,
                     int linkA[], int linkB[], LinkType linkType[], int* linkCount,
                     char itemCode[][cfg::kItemIdLen], char itemSymbol[], char itemName[][cfg::kNameLen],
                     int itemRoom[], ItemClass itemClass[], ItemId itemAs[], int itemValue[],
                     int* itemCount,
                     int grid[][cfg::kRoomsPerFloor],
                     char* error, int errorCap);

// Case-insensitive lookup by id, e.g. "f3-r2". Returns -1 when absent.
int findRoom(const char roomId[][cfg::kIdLen], int roomCount, const char* id);

// Guards the start room before the state is initialised.
bool validateStartingRoom(int roomCount, int roomIndex);

// Returns the link type joining two rooms, or LINK_NONE.
LinkType findLink(const int linkA[], const int linkB[], const LinkType linkType[],
                  int linkCount, int from, int to);

// Resolves an item by id, display name, or single-character symbol.
// Returns -1 when nothing matches.
int findItemDef(const char itemCode[][cfg::kItemIdLen], const char itemName[][cfg::kNameLen],
                const char itemSymbol[], int itemCount, const char* token);

// Returns the array index of an item id, or -1.
int findItemById(const ItemId itemAs[], int itemCount, ItemId id);

const char* linkTypeName(LinkType type);
const char* itemClassName(ItemClass cls);

// Writes "Exits: F3-R2 (locked door), F3-R4 (door)" into out[].
void buildExitList(const char roomId[][cfg::kIdLen], const int linkA[], const int linkB[],
                   const LinkType linkType[], int linkCount, int room, char* out, int cap);

#endif  // LEVEL_H