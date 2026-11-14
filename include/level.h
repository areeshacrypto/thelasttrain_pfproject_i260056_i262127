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

struct Room {
    char id[cfg::kIdLen];
    char name[cfg::kNameLen];
    int floorIdx;   // 0-based
    int roomIdx;    // 0-based
    int gridRow;    // from the GRID block in building.txt
    int gridCol;
};

struct Link {
    int a;
    int b;
    LinkType type;
};

struct ItemDef {
    char id[cfg::kItemIdLen];
    ItemId itemId;
    char symbol;
    char name[cfg::kNameLen];
    int room;
    ItemClass cls;
    int value;   // energy/time gain, or clue digit
};

struct BuildingMap {
    Room rooms[cfg::kMaxRooms];
    int roomCount;
    Link links[cfg::kMaxLinks];
    int linkCount;
    ItemDef items[cfg::kMaxItems];
    int itemCount;
    // 2D coordinate grid filled from the GRID block: row = floor, col = room.
    int roomGrid[cfg::kFloors][cfg::kRoomsPerFloor];
};

// Reads ./data/building.txt. Returns false and fills error[] on any problem;
// the map is reset before parsing so a failed load never leaves half data.
bool loadBuildingMap(const char* path, BuildingMap* map, char* error, int errorCap);

// Case-insensitive lookup by id, e.g. "f3-r2". Returns -1 when absent.
int findRoom(const BuildingMap* map, const char* roomId);

// Guards the start room before the state is initialised.
bool validateStartingRoom(const BuildingMap* map, int roomIndex);

// Returns the link type joining two rooms, or LINK_NONE.
LinkType findLink(const BuildingMap* map, int from, int to);

// Resolves an item by id, display name, or single-character symbol.
// Returns -1 when nothing matches.
int findItemDef(const BuildingMap* map, const char* token);

const ItemDef* findItemById(const BuildingMap* map, ItemId id);

const char* linkTypeName(LinkType type);
const char* itemClassName(ItemClass cls);

// Writes "Exits: F3-R2 (locked door), F3-R4 (door)" into out[].
void buildExitList(const BuildingMap* map, int room, char* out, int cap);

#endif  // LEVEL_H
