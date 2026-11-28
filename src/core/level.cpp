#include "level.h"
#include <cstdio>
#include <cstring>
#include "util.h"

using namespace std;

static const char* const kItemTokens[] = {
    "MASTER_KEY",
    "ENERGY_PACK_A",
    "ENERGY_PACK_B",
    "TOOLKIT",
    "CLUE_FOUR",
    "FUSE",
    "CAT",
    "PANEL",
    "LIFT_PASS",
    "PASSENGER",
    "MEDKIT",
    "ACCESS_CARD",
    "CLUE_TWO",
    "CLUE_SEVEN",
    "CABINET",
    "TRAIN_KEY",
    "CAT_CARRIER",
    "OBSERVATION_TOKEN",
};

static const ItemId kItemTokenIds[] = {
    IT_MASTER_KEY,
    IT_ENERGY_PACK_A,
    IT_ENERGY_PACK_B,
    IT_TOOLKIT,
    IT_CLUE_FOUR,
    IT_FUSE,
    IT_CAT,
    IT_PANEL,
    IT_LIFT_PASS,
    IT_PASSENGER,
    IT_MEDKIT,
    IT_ACCESS_CARD,
    IT_CLUE_TWO,
    IT_CLUE_SEVEN,
    IT_CABINET,
    IT_TRAIN_KEY,
    IT_CAT_CARRIER,
    IT_OBSERVATION_TOKEN,
};

static const int kItemTokenCount = (int)(sizeof(kItemTokens) / sizeof(kItemTokens[0]));

static void setError(char* error, int cap, const char* text) {
    util::copyText(error, cap, text);
}

// Reads the next valid data line, skipping empty lines and '#' comments
static bool nextDataLine(FILE* file, char* buffer, int cap) {
    while (fgets(buffer, cap, file) != nullptr) {
        util::trimInPlace(buffer);
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        return true;
    }
    return false;
}

static int splitFields(char* text, char separator, char* fields[], int maxFields) {
    int count = 0;
    fields[count++] = text;
    for (char* p = text; *p != '\0'; p++) {
        if (*p == separator) {
            *p = '\0';
            if (count < maxFields) {
                fields[count] = p + 1;
            }
            count++;
        }
    }
    return count;
}

static bool parseRoomId(const char* text, int* floorIdx, int* roomIdx) {
    if (text == nullptr) return false;
    if (text[0] != 'F' || text[1] < '1' || text[1] > '5') return false;
    int floor = text[1] - '0';

    if (text[2] != '-' || text[3] != 'R') return false;
    if (text[4] < '1' || text[4] > '4' || text[5] != '\0') return false;
    int room = text[4] - '0';

    *floorIdx = floor - 1;
    *roomIdx = room - 1;
    return true;
}

static bool parseLinkType(const char* text, LinkType* out) {
    if (util::equalsIgnoreCase(text, "DOOR")) *out = LINK_DOOR;
    else if (util::equalsIgnoreCase(text, "WINDOW")) *out = LINK_WINDOW;
    else if (util::equalsIgnoreCase(text, "STAIR")) *out = LINK_STAIR;
    else if (util::equalsIgnoreCase(text, "LOCKED")) *out = LINK_LOCKED;
    else if (util::equalsIgnoreCase(text, "LIFT1")) *out = LINK_LIFT1;
    else if (util::equalsIgnoreCase(text, "LIFT2")) *out = LINK_LIFT2;
    else if (util::equalsIgnoreCase(text, "WALL")) *out = LINK_WALL;
    else return false;
    return true;
}

static bool parseItemClass(const char* text, ItemClass* out) {
    if (util::equalsIgnoreCase(text, "KEY")) *out = CLASS_KEY;
    else if (util::equalsIgnoreCase(text, "ENERGY")) *out = CLASS_ENERGY;
    else if (util::equalsIgnoreCase(text, "TIME")) *out = CLASS_TIME;
    else if (util::equalsIgnoreCase(text, "CLUE")) *out = CLASS_CLUE;
    else if (util::equalsIgnoreCase(text, "QUEST")) *out = CLASS_QUEST;
    else if (util::equalsIgnoreCase(text, "ENTITY")) *out = CLASS_ENTITY;
    else if (util::equalsIgnoreCase(text, "FIXED")) *out = CLASS_FIXED;
    else return false;
    return true;
}

static bool parseIntAfter(const char* line, const char* keyword, int* value) {
    size_t len = strlen(keyword);
    if (strncmp(line, keyword, len) != 0) return false;

    const char* cursor = line + len;
    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }
    return util::parseIntStrict(cursor, value);
}

static bool itemIdFromToken(const char* text, ItemId* out) {
    for (int i = 0; i < kItemTokenCount; i++) {
        if (util::equalsIgnoreCase(text, kItemTokens[i])) {
            *out = kItemTokenIds[i];
            return true;
        }
    }
    return false;
}

static bool parseBuildingFile(FILE* file,
                              char roomId[][cfg::kIdLen], char roomName[][cfg::kNameLen],
                              int roomFloor[], int roomSlot[], int roomGridRow[], int roomGridCol[],
                              int* roomCount,
                              int linkA[], int linkB[], LinkType linkType[], int* linkCount,
                              char itemCode[][cfg::kItemIdLen], char itemSymbol[], char itemName[][cfg::kNameLen],
                              int itemRoom[], ItemClass itemClass[], ItemId itemAs[], int itemValue[],
                              int* itemCount,
                              int grid[][cfg::kRoomsPerFloor],
                              char* error, int errorCap) {
    char line[cfg::kMaxLineLen];
    char* fields[8];

    for (int row = 0; row < cfg::kFloors; row++) {
        for (int col = 0; col < cfg::kRoomsPerFloor; col++) {
            grid[row][col] = -1;
        }
    }

    if (!nextDataLine(file, line, sizeof(line)) || strcmp(line, "VERSION 1") != 0) {
        setError(error, errorCap, "building.txt: expected 'VERSION 1'");
        return false;
    }

    int declaredRooms = 0;
    if (!nextDataLine(file, line, sizeof(line)) || !parseIntAfter(line, "ROOMS", &declaredRooms)) {
        setError(error, errorCap, "building.txt: bad ROOMS header");
        return false;
    }
    if (declaredRooms != cfg::kMaxRooms) {
        setError(error, errorCap, "building.txt: ROOMS must be 20");
        return false;
    }

    int perFloor[cfg::kFloors] = {0, 0, 0, 0, 0};
    for (int i = 0; i < declaredRooms; i++) {
        if (!nextDataLine(file, line, sizeof(line))) {
            setError(error, errorCap, "building.txt: missing ROOM line");
            return false;
        }
        if (strncmp(line, "ROOM ", 5) != 0 || splitFields(line + 5, '|', fields, 3) != 2) {
            setError(error, errorCap, "building.txt: ROOM needs id|name");
            return false;
        }
        int idx = *roomCount;
        util::copyText(roomId[idx], cfg::kIdLen, fields[0]);
        util::trimInPlace(roomId[idx]);
        util::copyText(roomName[idx], cfg::kNameLen, fields[1]);
        util::trimInPlace(roomName[idx]);

        int floorIdx = 0;
        int slotIdx = 0;
        if (!parseRoomId(roomId[idx], &floorIdx, &slotIdx)) {
            setError(error, errorCap, "building.txt: invalid room id");
            return false;
        }
        roomFloor[idx] = floorIdx;
        roomSlot[idx] = slotIdx;
        roomGridRow[idx] = -1;
        roomGridCol[idx] = -1;

        for (int r = 0; r < *roomCount; r++) {
            if (util::equalsIgnoreCase(roomId[r], roomId[idx])) {
                setError(error, errorCap, "building.txt: duplicate room id");
                return false;
            }
        }
        (*roomCount)++;
        perFloor[floorIdx]++;
    }

    for (int f = 0; f < cfg::kFloors; f++) {
        if (perFloor[f] != cfg::kRoomsPerFloor) {
            setError(error, errorCap, "building.txt: each floor needs 4 rooms");
            return false;
        }
    }

    int declaredLinks = 0;
    if (!nextDataLine(file, line, sizeof(line)) || !parseIntAfter(line, "LINKS", &declaredLinks)) {
        setError(error, errorCap, "building.txt: bad LINKS header");
        return false;
    }

    for (int i = 0; i < declaredLinks; i++) {
        if (!nextDataLine(file, line, sizeof(line))) {
            setError(error, errorCap, "building.txt: missing LINK line");
            return false;
        }
        if (strncmp(line, "LINK ", 5) != 0 || splitFields(line + 5, '|', fields, 4) != 3) {
            setError(error, errorCap, "building.txt: LINK needs a|b|type");
            return false;
        }
        int a = findRoom(roomId, *roomCount, fields[0]);
        int b = findRoom(roomId, *roomCount, fields[1]);
        LinkType type = LINK_NONE;
        if (a < 0 || b < 0 || a == b || !parseLinkType(fields[2], &type)) {
            setError(error, errorCap, "building.txt: invalid LINK entry");
            return false;
        }
        if (*linkCount >= cfg::kMaxLinks) {
            setError(error, errorCap, "building.txt: too many links");
            return false;
        }
        linkA[*linkCount] = a;
        linkB[*linkCount] = b;
        linkType[*linkCount] = type;
        (*linkCount)++;
    }

    int declaredItems = 0;
    if (!nextDataLine(file, line, sizeof(line)) || !parseIntAfter(line, "ITEMS", &declaredItems)) {
        setError(error, errorCap, "building.txt: bad ITEMS header");
        return false;
    }
    if (declaredItems != IT_COUNT) {
        setError(error, errorCap, "building.txt: ITEMS must match item count");
        return false;
    }

    bool seenItem[IT_COUNT] = {false};
    for (int i = 0; i < declaredItems; i++) {
        if (!nextDataLine(file, line, sizeof(line))) {
            setError(error, errorCap, "building.txt: missing ITEM line");
            return false;
        }
        if (strncmp(line, "ITEM ", 5) != 0 || splitFields(line + 5, '|', fields, 7) != 6) {
            setError(error, errorCap, "building.txt: ITEM needs id|symbol|name|room|class|value");
            return false;
        }
        if (*itemCount >= cfg::kMaxItems) {
            setError(error, errorCap, "building.txt: too many items");
            return false;
        }
        int idx = *itemCount;
        util::copyText(itemCode[idx], cfg::kItemIdLen, fields[0]);
        util::trimInPlace(itemCode[idx]);
        util::trimInPlace(fields[1]);
        util::trimInPlace(fields[2]);
        util::trimInPlace(fields[3]);
        util::trimInPlace(fields[4]);
        util::trimInPlace(fields[5]);

        if (strlen(fields[1]) != 1) {
            setError(error, errorCap, "building.txt: item symbol must be 1 char");
            return false;
        }
        itemSymbol[idx] = fields[1][0];
        util::copyText(itemName[idx], cfg::kNameLen, fields[2]);
        itemRoom[idx] = findRoom(roomId, *roomCount, fields[3]);
        if (itemRoom[idx] < 0) {
            setError(error, errorCap, "building.txt: item room not found");
            return false;
        }
        if (!parseItemClass(fields[4], &itemClass[idx])) {
            setError(error, errorCap, "building.txt: unknown item class");
            return false;
        }
        if (!util::parseIntStrict(fields[5], &itemValue[idx])) {
            setError(error, errorCap, "building.txt: bad item value");
            return false;
        }
        ItemId as = IT_COUNT;
        if (!itemIdFromToken(itemCode[idx], &as) || seenItem[as]) {
            setError(error, errorCap, "building.txt: unknown or duplicate item id");
            return false;
        }
        seenItem[as] = true;
        itemAs[idx] = as;
        (*itemCount)++;
    }

    if (!nextDataLine(file, line, sizeof(line)) || strcmp(line, "GRID") != 0) {
        setError(error, errorCap, "building.txt: missing GRID block");
        return false;
    }
    for (int row = 0; row < cfg::kFloors; row++) {
        if (!nextDataLine(file, line, sizeof(line))) {
            setError(error, errorCap, "building.txt: GRID needs 5 rows");
            return false;
        }
        if (util::splitTokens(line, fields, cfg::kRoomsPerFloor + 1) != cfg::kRoomsPerFloor) {
            setError(error, errorCap, "building.txt: GRID row needs 4 rooms");
            return false;
        }
        for (int col = 0; col < cfg::kRoomsPerFloor; col++) {
            int index = findRoom(roomId, *roomCount, fields[col]);
            if (index < 0 || roomGridRow[index] != -1) {
                setError(error, errorCap, "building.txt: bad GRID cell");
                return false;
            }
            roomGridRow[index] = row;
            roomGridCol[index] = col;
            grid[row][col] = index;
        }
    }
    for (int i = 0; i < *roomCount; i++) {
        if (roomGridRow[i] < 0) {
            setError(error, errorCap, "building.txt: room missing from GRID");
            return false;
        }
    }
    return true;
}

bool loadBuildingMap(const char* path,
                     char roomId[][cfg::kIdLen], char roomName[][cfg::kNameLen],
                     int roomFloor[], int roomSlot[], int roomGridRow[], int roomGridCol[],
                     int* roomCount,
                     int linkA[], int linkB[], LinkType linkType[], int* linkCount,
                     char itemCode[][cfg::kItemIdLen], char itemSymbol[], char itemName[][cfg::kNameLen],
                     int itemRoom[], ItemClass itemClass[], ItemId itemAs[], int itemValue[],
                     int* itemCount,
                     int grid[][cfg::kRoomsPerFloor],
                     char* error, int errorCap) {
    *roomCount = 0;
    *linkCount = 0;
    *itemCount = 0;
    if (error != nullptr && errorCap > 0) {
        error[0] = '\0';
    }
    FILE* file = fopen(path, "r");
    if (file == nullptr) {
        setError(error, errorCap, "building.txt: cannot open file");
        return false;
    }
    bool ok = parseBuildingFile(file,
                                roomId, roomName, roomFloor, roomSlot, roomGridRow, roomGridCol,
                                roomCount,
                                linkA, linkB, linkType, linkCount,
                                itemCode, itemSymbol, itemName, itemRoom, itemClass, itemAs, itemValue,
                                itemCount,
                                grid,
                                error, errorCap);
    fclose(file);
    if (!ok) {
        *roomCount = 0;
        *linkCount = 0;
        *itemCount = 0;
    }
    return ok;
}

int findRoom(const char roomId[][cfg::kIdLen], int roomCount, const char* id) {
    if (id == nullptr) return -1;
    for (int i = 0; i < roomCount; i++) {
        if (util::equalsIgnoreCase(roomId[i], id)) {
            return i;
        }
    }
    return -1;
}

bool validateStartingRoom(int roomCount, int roomIndex) {
    return roomIndex >= 0 && roomIndex < roomCount;
}

LinkType findLink(const int linkA[], const int linkB[], const LinkType linkType[],
                  int linkCount, int from, int to) {
    for (int i = 0; i < linkCount; i++) {
        if ((linkA[i] == from && linkB[i] == to) || (linkB[i] == from && linkA[i] == to)) {
            return linkType[i];
        }
    }
    return LINK_NONE;
}

int findItemDef(const char itemCode[][cfg::kItemIdLen], const char itemName[][cfg::kNameLen],
                const char itemSymbol[], int itemCount, const char* token) {
    if (token == nullptr) return -1;
    bool singleSymbol = (strlen(token) == 1);
    for (int i = 0; i < itemCount; i++) {
        if (util::equalsIgnoreCase(itemCode[i], token) ||
            util::equalsIgnoreCase(itemName[i], token) ||
            (singleSymbol && itemSymbol[i] == token[0])) {
            return i;
        }
    }
    return -1;
}

int findItemById(const ItemId itemAs[], int itemCount, ItemId id) {
    for (int i = 0; i < itemCount; i++) {
        if (itemAs[i] == id) {
            return i;
        }
    }
    return -1;
}

const char* linkTypeName(LinkType type) {
    switch (type) {
        case LINK_DOOR:   return "door";
        case LINK_WINDOW: return "window";
        case LINK_STAIR:  return "staircase";
        case LINK_LOCKED: return "locked door";
        case LINK_LIFT1:  return "lift 1";
        case LINK_LIFT2:  return "lift 2";
        case LINK_WALL:   return "solid wall";
        default:          return "no passage";
    }
}

const char* itemClassName(ItemClass cls) {
    switch (cls) {
        case CLASS_KEY:    return "key";
        case CLASS_ENERGY: return "energy";
        case CLASS_TIME:   return "time";
        case CLASS_CLUE:   return "clue";
        case CLASS_QUEST:  return "quest";
        case CLASS_ENTITY: return "entity";
        case CLASS_FIXED:  return "fixed";
        default:           return "unknown";
    }
}

void buildExitList(const char roomId[][cfg::kIdLen], const int linkA[], const int linkB[],
                   const LinkType linkType[], int linkCount, int room, char* out, int cap) {
    if (out == nullptr || cap <= 0) return;
    util::copyText(out, cap, "Exits:");
    bool any = false;
    for (int i = 0; i < linkCount; i++) {
        int other = -1;
        if (linkA[i] == room) other = linkB[i];
        else if (linkB[i] == room) other = linkA[i];
        else continue;

        if (any) {
            util::appendStr(out, cap, ",");
        }
        util::appendStr(out, cap, " ");
        util::appendStr(out, cap, roomId[other]);
        util::appendStr(out, cap, " (");
        util::appendStr(out, cap, linkTypeName(linkType[i]));
        util::appendStr(out, cap, ")");
        any = true;
    }
    if (!any) {
        util::copyText(out, cap, "Exits: none");
    }
}