#include "viewer_iface.h"
#include <cstring>
#include <iomanip>
#include <iostream>
#include "config.h"
#include "level.h"
#include "state.h"
#include "util.h"

using namespace std;

static void makeBar(int value, char* out) {
    if (value < 0) value = 0;
    if (value > cfg::kResourceCap) value = cfg::kResourceCap;
    int filled = value / 10;
    for (int i = 0; i < 10; i++) {
        out[i] = (i < filled) ? '#' : '-';
    }
    out[10] = '\0';
}

static bool isSpent(const GameState* state, ItemId id) {
    if (id == IT_FUSE && state->fuseInstalled) return true;
    if (id == IT_MEDKIT && state->passengerRescued) return true;
    if (id == IT_CAT_CARRIER && state->catSecured) return true;
    return false;
}

static void appendItem(char* buffer, int cap, const char* text) {
    if (buffer[0] != '\0') {
        util::appendStr(buffer, cap, ", ");
    }
    util::appendStr(buffer, cap, text);
}

static int roomAtGrid(const BuildingMap* map, int row, int col) {
    if (row < 0 || row >= cfg::kFloors || col < 0 || col >= cfg::kRoomsPerFloor) {
        return -1;
    }
    return map->roomGrid[row][col];
}

void viewer::printBanner() {
    cout << "\n==============================================================\n";
    cout << "  THE LAST TRAIN  -  deterministic rescue simulation (C++17)\n";
    cout << "==============================================================\n";
}

void viewer::printStatus(const BuildingMap* map, const GameState* state) {
    const Room& room = map->rooms[state->room];
    char energyBar[11];
    char timeBar[11];
    makeBar(state->energy, energyBar);
    makeBar(state->time, timeBar);

    char inventory[256] = "";
    for (int i = 0; i < map->itemCount; i++) {
        const ItemDef& def = map->items[i];
        if (!state->itemTaken[def.itemId] || isSpent(state, def.itemId)) continue;
        if (def.cls == CLASS_ENERGY || def.cls == CLASS_TIME) continue;
        appendItem(inventory, (int)sizeof(inventory), def.name);
    }
    if (inventory[0] == '\0') {
        util::copyText(inventory, (int)sizeof(inventory), "(empty)");
    }

    char visible[256] = "";
    for (int i = 0; i < map->itemCount; i++) {
        const ItemDef& def = map->items[i];
        if (def.room != state->room || state->itemTaken[def.itemId]) continue;
        if (def.itemId == IT_TRAIN_KEY && !state->cabinetUnlocked) continue;

        char entry[64] = "";
        util::appendChar(entry, (int)sizeof(entry), def.symbol);
        util::appendChar(entry, (int)sizeof(entry), ' ');
        util::appendStr(entry, (int)sizeof(entry), def.name);
        appendItem(visible, (int)sizeof(visible), entry);
    }
    if (visible[0] == '\0') {
        util::copyText(visible, (int)sizeof(visible), "(nothing useful)");
    }

    char clues[16];
    formatClueCode(state, clues, (int)sizeof(clues));

    char exits[512];
    buildExitList(map, state->room, exits, (int)sizeof(exits));

    cout << "\n";
    cout << " Location : " << room.id << "  " << room.name << "\n";
    cout << " Energy   : [" << energyBar << "] " << setw(3) << state->energy << "/" << cfg::kResourceCap
         << "    Score : " << setw(3) << state->score << "/" << cfg::kMaxScore << "\n";
    cout << " Time     : [" << timeBar << "] " << setw(3) << state->time << "/" << cfg::kResourceCap
         << "    Turn  : " << state->turn << "\n";
    cout << " Inventory: " << inventory << "\n";
    cout << " Clues    : " << clues << "    Cabinet: " << (state->cabinetUnlocked ? "unlocked" : "locked") << "\n";
    cout << " You see  : " << visible << "\n";
    cout << " " << exits << "\n";
    cout << "\n";
}

void viewer::printMapOverview(const BuildingMap* map, const GameState* state) {
    cout << "\n Building map (* = you):\n";
    for (int row = 0; row < cfg::kFloors; row++) {
        cout << "  F" << (row + 1) << " |";
        for (int col = 0; col < cfg::kRoomsPerFloor; col++) {
            int index = roomAtGrid(map, row, col);
            if (index < 0) {
                cout << "   ??????";
                continue;
            }
            bool here = (index == state->room);
            cout << "  " << map->rooms[index].id << (here ? "*" : " ");
        }
        cout << "\n";
    }
    cout << "  Doors/staircases/lifts: use MOVE with an adjacent room id.\n";
}

void viewer::printHelp() {
    cout << "\nCOMMANDS\n"
         << "  LOOK                  describe the current room and status\n"
         << "  MAP                   show the building overview\n"
         << "  MOVE <room>           move to an adjacent room (e.g. MOVE F3-R2)\n"
         << "  INSPECT               search the current room          [-1E, -1T]\n"
         << "  COLLECT <item>        take an item by id, name or symbol [-1E, -1T]\n"
         << "  USE FUSE              install the Fuse at the panel    [-1E, -1T]\n"
         << "  USE MEDKIT            treat the injured passenger      [-1E, -1T]\n"
         << "  USE CAT_CARRIER       secure the cat in F3-R3          [-1E, -1T]\n"
         << "  UNLOCK CABINET <code> open the security cabinet        [-1E, -1T]\n"
         << "  BOARD                 board the train at F5-R2         [-1E, -1T]\n"
         << "  SAVE / LOAD           write or read out/savegame.txt\n"
         << "  STATUS / HELP / QUIT\n"
         << "\nMOVEMENT COSTS: door/stair -2E,-1T | window -3E,-2T | lift -4E,-3T\n";
}

void viewer::printResult(const BuildingMap* map, const GameState* state) {
    (void)map;
    cout << "\n==============================================================\n";
    cout << " RESULT : " << (state->victory ? "VICTORY" : "DEFEAT") << "\n";
    cout << " Score  : " << state->score << "/" << cfg::kMaxScore
         << "   Energy: " << state->energy << "/" << cfg::kResourceCap
         << "   Time: " << state->time << "/" << cfg::kResourceCap << "\n";
    cout << " Turns  : " << state->turn << "\n";
    cout << "==============================================================\n";
}