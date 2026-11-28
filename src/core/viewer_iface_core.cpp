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

static bool isSpent(ItemId id, bool fuseInstalled, bool passengerRescued, bool catSecured) {
    if (id == IT_FUSE && fuseInstalled) return true;
    if (id == IT_MEDKIT && passengerRescued) return true;
    if (id == IT_CAT_CARRIER && catSecured) return true;
    return false;
}

static void appendItem(char* buffer, int cap, const char* text) {
    if (buffer[0] != '\0') {
        util::appendStr(buffer, cap, ", ");
    }
    util::appendStr(buffer, cap, text);
}

static void appendItemLabel(char* buffer, int cap, const char* name, char symbol) {
    char label[96] = "";
    util::appendStr(label, (int)sizeof(label), name);
    util::appendStr(label, (int)sizeof(label), " (");
    util::appendChar(label, (int)sizeof(label), symbol);
    util::appendStr(label, (int)sizeof(label), ")");
    appendItem(buffer, cap, label);
}

static int roomAtGrid(const int grid[][cfg::kRoomsPerFloor], int row, int col) {
    if (row < 0 || row >= cfg::kFloors || col < 0 || col >= cfg::kRoomsPerFloor) {
        return -1;
    }
    return grid[row][col];
}

static void exitPhrase(LinkType type, int from, int to, const int roomFloor[],
                       const bool itemTaken[], bool fuseInstalled, char* out, int cap) {
    switch (type) {
        case LINK_DOOR:
            util::copyText(out, cap, "through a door");
            break;
        case LINK_WINDOW:
            util::copyText(out, cap, "through a window");
            break;
        case LINK_STAIR:
            util::copyText(out, cap, roomFloor[to] > roomFloor[from]
                                             ? "up the stairs"
                                             : "down the stairs");
            break;
        case LINK_LOCKED:
            util::copyText(out, cap, hasItem(itemTaken, IT_MASTER_KEY)
                                             ? "through the locked door"
                                             : "locked - needs the Master Key");
            break;
        case LINK_LIFT1:
            util::copyText(out, cap, "by lift 1");
            break;
        case LINK_LIFT2:
            util::copyText(out, cap, (fuseInstalled && hasItem(itemTaken, IT_LIFT_PASS))
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

void viewer::printStatus(int energy, int time, int score, int turn, int room,
                         const bool itemTaken[], bool fuseInstalled,
                         bool passengerRescued, bool catSecured,
                         bool clueFour, bool clueTwo, bool clueSeven, bool cabinetUnlocked,
                         const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                         const int roomFloor[], const int linkA[], const int linkB[],
                         const LinkType linkType[], int linkCount,
                         const char itemName[][cfg::kNameLen], const char itemSymbol[],
                         const int itemRoom[], const ItemClass itemClass[],
                         const ItemId itemAs[], int itemCount) {
    char energyBar[11];
    char timeBar[11];
    makeBar(energy, energyBar);
    makeBar(time, timeBar);

    char inventory[256] = "";
    for (int i = 0; i < itemCount; i++) {
        if (!itemTaken[itemAs[i]] || isSpent(itemAs[i], fuseInstalled, passengerRescued, catSecured)) continue;
        if (itemClass[i] == CLASS_ENERGY || itemClass[i] == CLASS_TIME) continue;
        appendItem(inventory, (int)sizeof(inventory), itemName[i]);
    }
    if (inventory[0] == '\0') {
        util::copyText(inventory, (int)sizeof(inventory), "nothing yet");
    }

    char visible[256] = "";
    for (int i = 0; i < itemCount; i++) {
        if (itemRoom[i] != room || itemTaken[itemAs[i]]) continue;
        if (itemAs[i] == IT_TRAIN_KEY && !cabinetUnlocked) continue;
        appendItemLabel(visible, (int)sizeof(visible), itemName[i], itemSymbol[i]);
    }
    if (visible[0] == '\0') {
        util::copyText(visible, (int)sizeof(visible), "nothing you can pick up");
    }

    char clues[16];
    formatClueCode(clueFour, clueTwo, clueSeven, clues, (int)sizeof(clues));

    cout << "\n";
    cout << " You are here: " << roomName[room] << " (" << roomId[room] << ")\n\n";
    cout << "   Energy  [" << energyBar << "] " << setw(3) << energy << "/" << cfg::kResourceCap
         << "      Score  " << score << "/" << cfg::kMaxScore << "\n";
    cout << "   Time    [" << timeBar << "] " << setw(3) << time << "/" << cfg::kResourceCap
         << "      Turn   " << turn << "\n\n";
    cout << "   Carrying : " << inventory << "\n";
    cout << "   Clues    : " << clues << "      Cabinet: "
         << (cabinetUnlocked ? "open" : "locked") << "\n";
    cout << "   You see  : " << visible << "\n";
    cout << "   To go    :\n";

    bool anyExit = false;
    for (int i = 0; i < linkCount; i++) {
        int to = -1;
        if (linkA[i] == room) {
            to = linkB[i];
        } else if (linkB[i] == room) {
            to = linkA[i];
        } else {
            continue;
        }
        char phrase[96];
        exitPhrase(linkType[i], room, to, roomFloor, itemTaken, fuseInstalled, phrase, (int)sizeof(phrase));
        cout << "     " << left << setw(kExitColumn) << phrase << right
             << " -> " << roomName[to] << " (" << roomId[to] << ")\n";
        anyExit = true;
    }
    if (!anyExit) {
        cout << "     nowhere - this room is sealed\n";
    }
    cout << "\n";
}

void viewer::printMapOverview(const int grid[][cfg::kRoomsPerFloor],
                              const char roomId[][cfg::kIdLen], int room) {
    cout << "\n The building (F1 is the ground floor, F5 the top; * = you):\n";
    for (int row = 0; row < cfg::kFloors; row++) {
        cout << "  F" << (row + 1) << " |";
        for (int col = 0; col < cfg::kRoomsPerFloor; col++) {
            int index = roomAtGrid(grid, row, col);
            if (index < 0) {
                cout << "   ??????";
                continue;
            }
            bool here = (index == room);
            cout << "  " << roomId[index] << (here ? "*" : " ");
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

void viewer::printResult(int energy, int time, int score, int turn, bool victory) {
    cout << "\n==============================================================\n";
    if (victory) {
        cout << "  VICTORY - you made the last train.\n";
    } else if (energy <= 0) {
        cout << "  DEFEAT - you ran out of energy.\n";
    } else {
        cout << "  DEFEAT - you ran out of time.\n";
    }
    cout << "  Score " << score << "/" << cfg::kMaxScore
         << "    Energy " << energy << "/" << cfg::kResourceCap
         << "    Time " << time << "/" << cfg::kResourceCap
         << "    Turns " << turn << "\n";
    cout << "==============================================================\n";
}