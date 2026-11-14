#include "viewer_iface.h"

#include <cstdio>
#include <cstring>

#include "config.h"
#include "level.h"
#include "state.h"
#include "util.h"

namespace {

void makeBar(int value, char* out) {
    if (value < 0) {
        value = 0;
    }
    if (value > cfg::kResourceCap) {
        value = cfg::kResourceCap;
    }
    const int filled = value / 10;
    for (int i = 0; i < 10; ++i) {
        out[i] = (i < filled) ? '#' : '-';
    }
    out[10] = '\0';
}

bool isSpent(const GameState* state, ItemId id) {
    if (id == IT_FUSE && state->fuseInstalled) {
        return true;
    }
    if (id == IT_MEDKIT && state->passengerRescued) {
        return true;
    }
    if (id == IT_CAT_CARRIER && state->catSecured) {
        return true;
    }
    return false;
}

void appendItem(char* buffer, int cap, const char* text) {
    const int len = static_cast<int>(std::strlen(buffer));
    if (len >= cap - 1) {
        return;
    }
    std::snprintf(buffer + len, static_cast<size_t>(cap - len), "%s%s",
                  len > 0 ? ", " : "", text);
}

int roomAtGrid(const BuildingMap* map, int row, int col) {
    if (row < 0 || row >= cfg::kFloors || col < 0 || col >= cfg::kRoomsPerFloor) {
        return -1;
    }
    return map->roomGrid[row][col];
}

}  // namespace

void viewer::printBanner() {
    std::printf("\n==============================================================\n");
    std::printf("  THE LAST TRAIN  -  deterministic rescue simulation (C++17)\n");
    std::printf("==============================================================\n");
}

void viewer::printStatus(const BuildingMap* map, const GameState* state) {
    const Room& room = map->rooms[state->room];
    char energyBar[11];
    char timeBar[11];
    makeBar(state->energy, energyBar);
    makeBar(state->time, timeBar);

    char inventory[256];
    inventory[0] = '\0';
    for (int i = 0; i < map->itemCount; ++i) {
        const ItemDef& def = map->items[i];
        if (!state->itemTaken[def.itemId] || isSpent(state, def.itemId)) {
            continue;
        }
        if (def.cls == CLASS_ENERGY || def.cls == CLASS_TIME) {
            continue;  // consumed on pickup
        }
        appendItem(inventory, static_cast<int>(sizeof inventory), def.name);
    }
    if (inventory[0] == '\0') {
        util::copyText(inventory, static_cast<int>(sizeof inventory), "(empty)");
    }

    char visible[256];
    visible[0] = '\0';
    for (int i = 0; i < map->itemCount; ++i) {
        const ItemDef& def = map->items[i];
        if (def.room != state->room || state->itemTaken[def.itemId]) {
            continue;
        }
        if (def.itemId == IT_TRAIN_KEY && !state->cabinetUnlocked) {
            continue;
        }
        char entry[64];
        std::snprintf(entry, sizeof entry, "%c %s", def.symbol, def.name);
        appendItem(visible, static_cast<int>(sizeof visible), entry);
    }
    if (visible[0] == '\0') {
        util::copyText(visible, static_cast<int>(sizeof visible), "(nothing useful)");
    }

    char clues[16];
    formatClueCode(state, clues, static_cast<int>(sizeof clues));

    char exits[512];
    buildExitList(map, state->room, exits, static_cast<int>(sizeof exits));

    std::printf("\n--------------------------------------------------------------\n");
    std::printf(" Location : %s  %s\n", room.id, room.name);
    std::printf(" Energy   : [%s] %3d/%d    Score : %3d/%d\n", energyBar,
                state->energy, cfg::kResourceCap, state->score, cfg::kMaxScore);
    std::printf(" Time     : [%s] %3d/%d    Turn  : %d\n", timeBar, state->time,
                cfg::kResourceCap, state->turn);
    std::printf(" Inventory: %s\n", inventory);
    std::printf(" Clues    : %s    Cabinet: %s\n", clues,
                state->cabinetUnlocked ? "unlocked" : "locked");
    std::printf(" You see  : %s\n", visible);
    std::printf(" %s\n", exits);
    std::printf("--------------------------------------------------------------\n");
}

void viewer::printMapOverview(const BuildingMap* map, const GameState* state) {
    std::printf("\n Building map (* = you):\n");
    for (int row = 0; row < cfg::kFloors; ++row) {
        std::printf("  F%d |", row + 1);
        for (int col = 0; col < cfg::kRoomsPerFloor; ++col) {
            const int index = roomAtGrid(map, row, col);
            if (index < 0) {
                std::printf("   ??????");
                continue;
            }
            const bool here = (index == state->room);
            std::printf("  %s%s", map->rooms[index].id, here ? "*" : " ");
        }
        std::printf("\n");
    }
    std::printf("  Doors/staircases/lifts: use MOVE with an adjacent room id.\n");
}

void viewer::printHelp() {
    std::printf(
        "\nCOMMANDS\n"
        "  LOOK                  describe the current room and status\n"
        "  MAP                   show the building overview\n"
        "  MOVE <room>           move to an adjacent room (e.g. MOVE F3-R2)\n"
        "  INSPECT               search the current room          [-1E, -1T]\n"
        "  COLLECT <item>        take an item by id, name or symbol [-1E, -1T]\n"
        "  USE FUSE              install the Fuse at the panel    [-1E, -1T]\n"
        "  USE MEDKIT            treat the injured passenger      [-1E, -1T]\n"
        "  USE CAT_CARRIER       secure the cat in F3-R3          [-1E, -1T]\n"
        "  UNLOCK CABINET <code> open the security cabinet        [-1E, -1T]\n"
        "  BOARD                 board the train at F5-R2         [-1E, -1T]\n"
        "  SAVE / LOAD           write or read out/savegame.txt\n"
        "  STATUS / HELP / QUIT\n"
        "\nMOVEMENT COSTS: door/stair -2E,-1T | window -3E,-2T | lift -4E,-3T\n");
}

void viewer::printResult(const BuildingMap* map, const GameState* state) {
    (void)map;
    std::printf("\n==============================================================\n");
    std::printf(" RESULT : %s\n", state->victory ? "VICTORY" : "DEFEAT");
    std::printf(" Score  : %d/%d   Energy: %d/%d   Time: %d/%d\n", state->score,
                cfg::kMaxScore, state->energy, cfg::kResourceCap, state->time,
                cfg::kResourceCap);
    std::printf(" Turns  : %d\n", state->turn);
    std::printf("==============================================================\n");
}
