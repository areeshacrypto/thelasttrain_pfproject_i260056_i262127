#include "viewer_iface.h"
#include <cstring>
#include <iomanip>
#include <iostream>
#include "config.h"
#include "level.h"
#include "state.h"
#include "util.h"

using namespace std;

static const int kExitColumn = 30;

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

static void appendItemLabel(char* buffer, int cap, const ItemDef& def) {
    char label[96] = "";
    util::appendStr(label, (int)sizeof(label), def.name);
    util::appendStr(label, (int)sizeof(label), " (");
    util::appendChar(label, (int)sizeof(label), def.symbol);
    util::appendStr(label, (int)sizeof(label), ")");
    appendItem(buffer, cap, label);
}

static int roomAtGrid(const BuildingMap* map, int row, int col) {
    if (row < 0 || row >= cfg::kFloors || col < 0 || col >= cfg::kRoomsPerFloor) {
        return -1;
    }
    return map->roomGrid[row][col];
}

static void exitPhrase(const BuildingMap* map, const GameState* state, LinkType type,
                       int from, int to, char* out, int cap) {
    const Room& target = map->rooms[to];
    switch (type) {
        case LINK_DOOR:
            util::copyText(out, cap, "through a door");
            break;
        case LINK_WINDOW:
            util::copyText(out, cap, "through a window");
            break;
        case LINK_STAIR:
            util::copyText(out, cap, target.floorIdx > map->rooms[from].floorIdx
                                             ? "up the stairs"
                                             : "down the stairs");
            break;
        case LINK_LOCKED:
            util::copyText(out, cap, hasItem(state, IT_MASTER_KEY)
                                             ? "through the locked door"
                                             : "locked - needs the Master Key");
            break;
        case LINK_LIFT1:
            util::copyText(out, cap, "by lift 1");
            break;
        case LINK_LIFT2:
            util::copyText(out, cap, (state->fuseInstalled && hasItem(state, IT_LIFT_PASS))
                                             ? "by lift 2"
                                             : "lift 2 - no power");
            break;
        case LINK_WALL:
            util::copyText(out, cap, "blocked by a solid wall");
            break;
        default:
            util::copyText(out, cap, "no way through");
            break;
    }
}

void viewer::printBanner() {
    cout << "\n==============================================================\n";
    cout << "  THE LAST TRAIN\n";
    cout << "  A five-floor rescue on the last night of service.\n";
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
        util::copyText(inventory, (int)sizeof(inventory), "nothing yet");
    }

    char visible[256] = "";
    for (int i = 0; i < map->itemCount; i++) {
        const ItemDef& def = map->items[i];
        if (def.room != state->room || state->itemTaken[def.itemId]) continue;
        if (def.itemId == IT_TRAIN_KEY && !state->cabinetUnlocked) continue;
        appendItemLabel(visible, (int)sizeof(visible), def);
    }
    if (visible[0] == '\0') {
        util::copyText(visible, (int)sizeof(visible), "nothing you can pick up");
    }

    char clues[16];
    formatClueCode(state, clues, (int)sizeof(clues));

    cout << "\n";
    cout << " You are here: " << room.name << " (" << room.id << ")\n\n";
    cout << "   Energy  [" << energyBar << "] " << setw(3) << state->energy << "/" << cfg::kResourceCap
         << "      Score  " << state->score << "/" << cfg::kMaxScore << "\n";
    cout << "   Time    [" << timeBar << "] " << setw(3) << state->time << "/" << cfg::kResourceCap
         << "      Turn   " << state->turn << "\n\n";
    cout << "   Carrying : " << inventory << "\n";
    cout << "   Clues    : " << clues << "      Cabinet: "
         << (state->cabinetUnlocked ? "open" : "locked") << "\n";
    cout << "   You see  : " << visible << "\n";
    cout << "   To go    :\n";

    bool anyExit = false;
    for (int i = 0; i < map->linkCount; i++) {
        const Link& link = map->links[i];
        int to = -1;
        if (link.a == state->room) {
            to = link.b;
        } else if (link.b == state->room) {
            to = link.a;
        } else {
            continue;
        }
        char phrase[96];
        exitPhrase(map, state, link.type, state->room, to, phrase, (int)sizeof(phrase));
        cout << "     " << left << setw(kExitColumn) << phrase << right
             << " -> " << map->rooms[to].name << " (" << map->rooms[to].id << ")\n";
        anyExit = true;
    }
    if (!anyExit) {
        cout << "     nowhere - this room is sealed\n";
    }
    cout << "\n";
}

void viewer::printMapOverview(const BuildingMap* map, const GameState* state) {
    cout << "\n The building (F1 is the ground floor, F5 the top; * = you):\n";
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
    cout << "  Type MOVE <room id> to walk to a room next to yours.\n";
}

void viewer::printHelp() {
    cout << "\nHOW TO PLAY\n"
         << "  LOOK / L              look around and show your status\n"
         << "  MAP                   show the five floors\n"
         << "  MOVE <room id>        walk to a room next to yours (e.g. MOVE F3-R2)\n"
         << "  INSPECT / SEARCH      search the room             1 energy, 1 time\n"
         << "  COLLECT <item>        pick something up           1 energy, 1 time\n"
         << "  USE FUSE              fit the Fuse into the panel 1 energy, 1 time\n"
         << "  USE MEDKIT            treat the injured passenger 1 energy, 1 time\n"
         << "  USE CAT_CARRIER       put the cat in the carrier  1 energy, 1 time\n"
         << "  UNLOCK CABINET 427    open the security cabinet   1 energy, 1 time\n"
         << "  BOARD                 get on the train at F5-R2   1 energy, 1 time\n"
         << "  SAVE / LOAD           keep or restore out/savegame.txt\n"
         << "  QUIT                  leave the building\n"
         << "\nWalking costs: door or stairs 2 energy/1 time, window 3/2, lift 4/3.\n"
         << "Energy and time both start at 100. If either reaches 0, you lose.\n";
}

void viewer::printResult(const BuildingMap* map, const GameState* state) {
    (void)map;
    cout << "\n==============================================================\n";
    if (state->victory) {
        cout << "  VICTORY - you made the last train.\n";
    } else if (state->energy <= 0) {
        cout << "  DEFEAT - you ran out of energy.\n";
    } else {
        cout << "  DEFEAT - you ran out of time.\n";
    }
    cout << "  Score " << state->score << "/" << cfg::kMaxScore
         << "    Energy " << state->energy << "/" << cfg::kResourceCap
         << "    Time " << state->time << "/" << cfg::kResourceCap
         << "    Turns " << state->turn << "\n";
    cout << "==============================================================\n";
}
