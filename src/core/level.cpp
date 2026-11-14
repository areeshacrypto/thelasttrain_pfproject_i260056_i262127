#include "level.h"
#include <cstdio>
#include <cstring>
#include "util.h"

using namespace std;

struct ItemToken {
    const char* token;
    ItemId id;
};

static const ItemToken kItemTokens[] = {
    {"MASTER_KEY", IT_MASTER_KEY},
    {"ENERGY_PACK_A", IT_ENERGY_PACK_A},
    {"ENERGY_PACK_B", IT_ENERGY_PACK_B},
    {"TOOLKIT", IT_TOOLKIT},
    {"CLUE_FOUR", IT_CLUE_FOUR},
    {"FUSE", IT_FUSE},
    {"CAT", IT_CAT},
    {"PANEL", IT_PANEL},
    {"LIFT_PASS", IT_LIFT_PASS},
    {"PASSENGER", IT_PASSENGER},
    {"MEDKIT", IT_MEDKIT},
    {"ACCESS_CARD", IT_ACCESS_CARD},
    {"CLUE_TWO", IT_CLUE_TWO},
    {"CLUE_SEVEN", IT_CLUE_SEVEN},
    {"CABINET", IT_CABINET},
    {"TRAIN_KEY", IT_TRAIN_KEY},
    {"CAT_CARRIER", IT_CAT_CARRIER},
    {"OBSERVATION_TOKEN", IT_OBSERVATION_TOKEN},
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
        if (util::equalsIgnoreCase(text, kItemTokens[i].token)) {
            *out = kItemTokens[i].id;
            return true;
        }
    }
    return false;
}

static bool parseBuildingFile(FILE* file, BuildingMap* map, char* error, int errorCap) {
    char line[cfg::kMaxLineLen];
    char* fields[8];

    for (int row = 0; row < cfg::kFloors; row++) {
        for (int col = 0; col < cfg::kRoomsPerFloor; col++) {
            map->roomGrid[row][col] = -1;
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
        Room room = {};
        util::copyText(room.id, cfg::kIdLen, fields[0]);
        util::trimInPlace(room.id);
        util::copyText(room.name, cfg::kNameLen, fields[1]);
        util::trimInPlace(room.name);

        if (!parseRoomId(room.id, &room.floorIdx, &room.roomIdx)) {
            setError(error, errorCap, "building.txt: invalid room id");
            return false;
        }
        room.gridRow = -1;
        room.gridCol = -1;

        for (int r = 0; r < map->roomCount; r++) {
            if (util::equalsIgnoreCase(map->rooms[r].id, room.id)) {
                setError(error, errorCap, "building.txt: duplicate room id");
                return false;
            }
        }
        map->rooms[map->roomCount++] = room;
        perFloor[room.floorIdx]++;
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
        int a = findRoom(map, fields[0]);
        int b = findRoom(map, fields[1]);
        LinkType type = LINK_NONE;
        if (a < 0 || b < 0 || a == b || !parseLinkType(fields[2], &type)) {
            setError(error, errorCap, "building.txt: invalid LINK entry");
            return false;
        }
        if (map->linkCount >= cfg::kMaxLinks) {
            setError(error, errorCap, "building.txt: too many links");
            return false;
        }
        map->links[map->linkCount].a = a;
        map->links[map->linkCount].b = b;
        map->links[map->linkCount].type = type;
        map->linkCount++;
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
        ItemDef def = {};
        util::copyText(def.id, cfg::kItemIdLen, fields[0]);
        util::trimInPlace(def.id);
        util::trimInPlace(fields[1]);
        util::trimInPlace(fields[2]);
        util::trimInPlace(fields[3]);
        util::trimInPlace(fields[4]);
        util::trimInPlace(fields[5]);

        if (strlen(fields[1]) != 1) {
            setError(error, errorCap, "building.txt: item symbol must be 1 char");
            return false;
        }
        def.symbol = fields[1][0];
        util::copyText(def.name, cfg::kNameLen, fields[2]);
        def.room = findRoom(map, fields[3]);
        if (def.room < 0) {
            setError(error, errorCap, "building.txt: item room not found");
            return false;
        }
        if (!parseItemClass(fields[4], &def.cls)) {
            setError(error, errorCap, "building.txt: unknown item class");
            return false;
        }
        if (!util::parseIntStrict(fields[5], &def.value)) {
            setError(error, errorCap, "building.txt: bad item value");
            return false;
        }
        if (!itemIdFromToken(def.id, &def.itemId) || seenItem[def.itemId]) {
            setError(error, errorCap, "building.txt: unknown or duplicate item id");
            return false;
        }
        seenItem[def.itemId] = true;
        if (map->itemCount >= cfg::kMaxItems) {
            setError(error, errorCap, "building.txt: too many items");
            return false;
        }
        map->items[map->itemCount++] = def;
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
            int index = findRoom(map, fields[col]);
            if (index < 0 || map->rooms[index].gridRow != -1) {
                setError(error, errorCap, "building.txt: bad GRID cell");
                return false;
            }
            map->rooms[index].gridRow = row;
            map->rooms[index].gridCol = col;
            map->roomGrid[row][col] = index;
        }
    }
    for (int i = 0; i < map->roomCount; i++) {
        if (map->rooms[i].gridRow < 0) {
            setError(error, errorCap, "building.txt: room missing from GRID");
            return false;
        }
    }
    return true;
}

bool loadBuildingMap(const char* path, BuildingMap* map, char* error, int errorCap) {
    *map = BuildingMap{};
    if (error != nullptr && errorCap > 0) {
        error[0] = '\0';
    }
    FILE* file = fopen(path, "r");
    if (file == nullptr) {
        setError(error, errorCap, "building.txt: cannot open file");
        return false;
    }
    bool ok = parseBuildingFile(file, map, error, errorCap);
    fclose(file);
    if (!ok) {
        *map = BuildingMap{};
    }
    return ok;
}

int findRoom(const BuildingMap* map, const char* roomId) {
    if (map == nullptr || roomId == nullptr) return -1;
    for (int i = 0; i < map->roomCount; i++) {
        if (util::equalsIgnoreCase(map->rooms[i].id, roomId)) {
            return i;
        }
    }
    return -1;
}

bool validateStartingRoom(const BuildingMap* map, int roomIndex) {
    return map != nullptr && roomIndex >= 0 && roomIndex < map->roomCount;
}

LinkType findLink(const BuildingMap* map, int from, int to) {
    for (int i = 0; i < map->linkCount; i++) {
        const Link& link = map->links[i];
        if ((link.a == from && link.b == to) || (link.b == from && link.a == to)) {
            return link.type;
        }
    }
    return LINK_NONE;
}

int findItemDef(const BuildingMap* map, const char* token) {
    if (map == nullptr || token == nullptr) return -1;
    bool singleSymbol = (strlen(token) == 1);
    for (int i = 0; i < map->itemCount; i++) {
        const ItemDef& def = map->items[i];
        if (util::equalsIgnoreCase(def.id, token) ||
            util::equalsIgnoreCase(def.name, token) ||
            (singleSymbol && def.symbol == token[0])) {
            return i;
        }
    }
    return -1;
}

const ItemDef* findItemById(const BuildingMap* map, ItemId id) {
    for (int i = 0; i < map->itemCount; i++) {
        if (map->items[i].itemId == id) {
            return &map->items[i];
        }
    }
    return nullptr;
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

void buildExitList(const BuildingMap* map, int room, char* out, int cap) {
    if (out == nullptr || cap <= 0) return;
    util::copyText(out, cap, "Exits:");
    bool any = false;
    for (int i = 0; i < map->linkCount; i++) {
        const Link& link = map->links[i];
        int other = -1;
        if (link.a == room) other = link.b;
        else if (link.b == room) other = link.a;
        else continue;

        if (any) {
            util::appendStr(out, cap, ",");
        }
        util::appendStr(out, cap, " ");
        util::appendStr(out, cap, map->rooms[other].id);
        util::appendStr(out, cap, " (");
        util::appendStr(out, cap, linkTypeName(link.type));
        util::appendStr(out, cap, ")");
        any = true;
    }
    if (!any) {
        util::copyText(out, cap, "Exits: none");
    }
}