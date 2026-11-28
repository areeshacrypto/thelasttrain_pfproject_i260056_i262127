#include "simulator.h"
#include <cstring>
#include <iostream>
#include "config.h"
#include "level.h"
#include "logger.h"
#include "physics.h"
#include "state.h"
#include "switches.h"
#include "util.h"
#include "viewer_iface.h"

using namespace std;

static const int kNoArgument = -1000000;

static void reportAction(int turn, const char* message) {
    cout << message << "\n";
    char entry[cfg::kAuditLineLen] = "Turn ";
    util::appendInt(entry, cfg::kAuditLineLen, turn);
    util::appendStr(entry, cfg::kAuditLineLen, ": ");
    util::appendStr(entry, cfg::kAuditLineLen, message);
    logger::addAudit(entry);
}

static void showStatus(int energy, int time, int score, int turn, int room,
                       const bool itemTaken[], bool fuseInstalled,
                       bool passengerRescued, bool catSecured,
                       bool clueFour, bool clueTwo, bool clueSeven, bool cabinetUnlocked,
                       const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                       const int roomFloor[], const int linkA[], const int linkB[],
                       const LinkType linkType[], int linkCount,
                       const char itemName[][cfg::kNameLen], const char itemSymbol[],
                       const int itemRoom[], const ItemClass itemClass[],
                       const ItemId itemAs[], int itemCount) {
    viewer::printStatus(energy, time, score, turn, room, itemTaken, fuseInstalled,
                        passengerRescued, catSecured, clueFour, clueTwo, clueSeven,
                        cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB,
                        linkType, linkCount, itemName, itemSymbol, itemRoom, itemClass,
                        itemAs, itemCount);
}

static void handleInspectCommand(int* energy, int* time, int* turn, int room,
                                 const bool itemTaken[], bool fuseInstalled,
                                 bool cabinetUnlocked, bool catSecured, bool passengerRescued,
                                 const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                                 const char itemName[][cfg::kNameLen], const char itemSymbol[],
                                 const int itemRoom[], const ItemClass itemClass[],
                                 const ItemId itemAs[], int itemCount,
                                 bool* over, bool* victory) {
    int costEnergy = 0;
    int costTime = 0;
    costInspect(&costEnergy, &costTime);
    applyCost(energy, time, costEnergy, costTime);
    (*turn)++;
    clampResources(energy, time);

    cout << "You inspect " << roomId[room] << " (" << roomName[room] << ").\n";

    bool found = false;
    for (int i = 0; i < itemCount; i++) {
        if (itemRoom[i] != room || itemTaken[itemAs[i]]) continue;
        if (itemAs[i] == IT_TRAIN_KEY && !cabinetUnlocked) continue;
        cout << "  - " << itemSymbol[i] << " " << itemName[i] << " (" << itemClassName(itemClass[i]) << ")\n";
        found = true;
    }
    if (!found) {
        cout << "  Nothing else of interest here.\n";
    }

    for (int i = 0; i < itemCount; i++) {
        if (itemRoom[i] != room) continue;
        switch (itemAs[i]) {
            case IT_PANEL:
                cout << "  The control panel is " << (fuseInstalled ? "powered" : "dark; it needs a Fuse") << ".\n";
                break;
            case IT_CABINET:
                cout << "  The security cabinet is " << (cabinetUnlocked ? "open" : "locked by a 3-digit keypad") << ".\n";
                break;
            case IT_CAT:
                cout << "  The cat is " << (catSecured ? "safe in the carrier" : "watching you") << ".\n";
                break;
            case IT_PASSENGER:
                cout << "  The injured passenger is " << (passengerRescued ? "ready to move" : "waiting for help") << ".\n";
                break;
            default:
                break;
        }
    }

    char entry[cfg::kAuditLineLen] = "Turn ";
    util::appendInt(entry, cfg::kAuditLineLen, *turn);
    util::appendStr(entry, cfg::kAuditLineLen, ": inspected ");
    util::appendStr(entry, cfg::kAuditLineLen, roomId[room]);
    logger::addAudit(entry);
    resolveDefeat(energy, time, over, victory);
}

static bool handleMoveCommand(int* energy, int* time, int* room, int* emergencyTrips, int* turn,
                              int roomCount,
                              const bool itemTaken[], bool fuseInstalled,
                              bool* over, bool* victory,
                              const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                              const int roomFloor[], const int linkA[], const int linkB[],
                              const LinkType linkType[], int linkCount,
                              char* tokens[], int count) {
    if (count < 2) {
        cout << "Usage: MOVE <room>   (e.g. MOVE F3-R2)\n";
        return false;
    }
    int target = findRoom(roomId, roomCount, tokens[1]);
    if (target < 0) {
        cout << "Unknown room '" << tokens[1] << "'. Type MAP to see the building.\n";
        return false;
    }
    char message[512];
    if (switches::tryTraverse(energy, time, room, emergencyTrips, turn,
                              itemTaken, fuseInstalled, over, victory,
                              roomId, roomName, roomFloor, linkA, linkB, linkType,
                              linkCount, roomCount, target, message, (int)sizeof(message))) {
        reportAction(*turn, message);
        return true;
    }
    cout << message << "\n";
    return false;
}

static bool handleCollectCommand(int* energy, int* time, int* score, int* room, int* turn,
                                 int emergencyTrips,
                                 bool itemTaken[], bool* clueFour, bool* clueTwo, bool* clueSeven,
                                 bool cabinetUnlocked, bool* over, bool* victory,
                                 const char roomId[][cfg::kIdLen],
                                 const char itemCode[][cfg::kItemIdLen],
                                 const char itemName[][cfg::kNameLen], const char itemSymbol[],
                                 const int itemRoom[], const ItemClass itemClass[],
                                 const ItemId itemAs[], const int itemValue[], int itemCount,
                                 char* tokens[], int count) {
    if (count < 2) {
        cout << "Usage: COLLECT <item>   (id, name, or symbol)\n";
        return false;
    }
    char message[512];
    if (switches::tryCollect(energy, time, score, room, turn, emergencyTrips,
                             itemTaken, clueFour, clueTwo, clueSeven,
                             cabinetUnlocked, over, victory,
                             roomId, itemCode, itemName, itemSymbol, itemRoom, itemClass,
                             itemAs, itemValue, itemCount, tokens[1], message, (int)sizeof(message))) {
        reportAction(*turn, message);
        return true;
    }
    cout << message << "\n";
    return false;
}

static bool handleUseCommand(int* energy, int* time, int* score, int* room, int* turn,
                             bool itemTaken[], bool* clueFour, bool* clueTwo, bool* clueSeven,
                             bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
                             bool* catSecured, bool* trainBoarded, bool* over, bool* victory,
                             int roomCount, const char roomId[][cfg::kIdLen],
                             const char itemCode[][cfg::kItemIdLen],
                             const char itemName[][cfg::kNameLen], const char itemSymbol[],
                             const int itemRoom[], const ItemId itemAs[], int itemCount,
                             char* tokens[], int count) {
    if (count < 2) {
        cout << "Usage: USE <item> [argument]\n";
        return false;
    }
    const char* target = tokens[1];
    ItemId id = IT_COUNT;
    if (util::equalsIgnoreCase(target, "TRAIN") ||
        util::equalsIgnoreCase(target, "BOARD") ||
        util::equalsIgnoreCase(target, "TRAIN_KEY")) {
        id = IT_TRAIN_KEY;
    } else if (util::equalsIgnoreCase(target, "MEDICAL_KIT")) {
        id = IT_MEDKIT;
    } else if (util::equalsIgnoreCase(target, "CARRIER")) {
        id = IT_CAT_CARRIER;
    } else {
        int index = findItemDef(itemCode, itemName, itemSymbol, itemCount, target);
        if (index < 0) {
            cout << "Unknown item '" << target << "'.\n";
            return false;
        }
        id = itemAs[index];
    }

    int argument = kNoArgument;
    if (count >= 3 && !util::parseIntStrict(tokens[2], &argument)) {
        cout << "'" << tokens[2] << "' is not a valid code.\n";
        return false;
    }
    if (id == IT_CABINET && count < 3) {
        cout << "Usage: UNLOCK CABINET <code>\n";
        return false;
    }

    char message[512];
    if (switches::tryUse(energy, time, score, room, turn, itemTaken,
                         clueFour, clueTwo, clueSeven, fuseInstalled, cabinetUnlocked,
                         passengerRescued, catSecured, trainBoarded, over, victory,
                         roomId, roomCount, itemName, itemRoom, itemAs, itemCount,
                         id, argument, message, (int)sizeof(message))) {
        reportAction(*turn, message);
        return true;
    }
    cout << message << "\n";
    return false;
}

static void handleSaveCommand(int energy, int time, int score, int room, int turn,
                              int emergencyTrips,
                              bool fuseInstalled, bool cabinetUnlocked, bool passengerRescued,
                              bool catSecured, bool trainBoarded,
                              const bool itemTaken[],
                              const char roomId[][cfg::kIdLen],
                              const char itemCode[][cfg::kItemIdLen],
                              const ItemId itemAs[], int itemCount) {
    char error[256];
    if (logger::saveGameStateToFile(energy, time, score, room, turn, emergencyTrips,
                                    fuseInstalled, cabinetUnlocked, passengerRescued,
                                    catSecured, trainBoarded, itemTaken, roomId, itemCode,
                                    itemAs, itemCount, cfg::kPathSaveGame, error, (int)sizeof(error))) {
        cout << "Game saved to " << cfg::kPathSaveGame << "\n";
    } else {
        cout << "Save failed: " << error << "\n";
    }
}

static void handleLoadCommand(int* energy, int* time, int* score, int* room, int* turn,
                              int* emergencyTrips,
                              bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
                              bool* catSecured, bool* trainBoarded,
                              bool* clueFour, bool* clueTwo, bool* clueSeven,
                              bool itemTaken[], bool* over, bool* victory,
                              const char roomId[][cfg::kIdLen], int roomCount,
                              const char roomName[][cfg::kNameLen], const int roomFloor[],
                              const int linkA[], const int linkB[], const LinkType linkType[],
                              int linkCount, const char itemCode[][cfg::kItemIdLen],
                              const char itemSymbol[], const char itemName[][cfg::kNameLen],
                              const int itemRoom[], const ItemClass itemClass[],
                              const ItemId itemAs[], int itemCount) {
    char error[256];
    if (logger::loadGameStateFromFile(energy, time, score, room, turn, emergencyTrips,
                                      fuseInstalled, cabinetUnlocked, passengerRescued,
                                      catSecured, trainBoarded, clueFour, clueTwo, clueSeven,
                                      itemTaken, over, victory, roomId, roomCount, itemCode,
                                      itemName, itemSymbol, itemAs, itemCount,
                                      cfg::kPathSaveGame, error, (int)sizeof(error))) {
        cout << "Game loaded from " << cfg::kPathSaveGame << "\n";
        showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
                   *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
                   *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
                   linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);
    } else {
        cout << "Load failed: " << error << "\n";
    }
}

int simulator::runSimulation(int* energy, int* time, int* score, int* room,
                             int* emergencyTrips, int* turn,
                             bool itemTaken[],
                             bool* clueFour, bool* clueTwo, bool* clueSeven,
                             bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
                             bool* catSecured, bool* trainBoarded, bool* over, bool* victory,
                             const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                             const int roomFloor[],
                             const int linkA[], const int linkB[], const LinkType linkType[],
                             int linkCount, int roomCount,
                             const char itemCode[][cfg::kItemIdLen], const char itemSymbol[],
                             const char itemName[][cfg::kNameLen], const int itemRoom[],
                             const ItemClass itemClass[], const ItemId itemAs[],
                             const int itemValue[], int itemCount,
                             const int grid[][cfg::kRoomsPerFloor],
                             std::FILE* input, bool echoCommands) {
    cout << "\nType HELP for the command list.\n";
    showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
               *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
               *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
               linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);

    bool quit = false;
    char line[cfg::kMaxLineLen];

    while (!*over && !quit) {
        cout << "> ";
        cout.flush();
        if (fgets(line, sizeof(line), input) == nullptr) {
            break;
        }
        if (echoCommands) {
            cout << line;
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] != '\n') {
                cout << "\n";
            }
            cout.flush();
        }

        util::trimInPlace(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        char* tokens[cfg::kMaxTokens] = {};
        int count = util::splitTokens(line, tokens, cfg::kMaxTokens);
        if (count == 0) continue;

        util::toUpperInPlace(tokens[0]);
        const char* command = tokens[0];

        if (strcmp(command, "HELP") == 0 || strcmp(command, "H") == 0 || strcmp(command, "?") == 0) {
            viewer::printHelp();
        } else if (strcmp(command, "LOOK") == 0 || strcmp(command, "L") == 0 || strcmp(command, "STATUS") == 0) {
            showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
                       *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
                       *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
                       linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);
        } else if (strcmp(command, "MAP") == 0) {
            viewer::printMapOverview(grid, roomId, *room);
        } else if (strcmp(command, "INSPECT") == 0 || strcmp(command, "SEARCH") == 0) {
            handleInspectCommand(energy, time, turn, *room, itemTaken, *fuseInstalled,
                                 *cabinetUnlocked, *catSecured, *passengerRescued,
                                 roomId, roomName, itemName, itemSymbol, itemRoom, itemClass,
                                 itemAs, itemCount, over, victory);
            if (!*over) {
                showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
                           *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
                           *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
                           linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);
            }
        } else if (strcmp(command, "MOVE") == 0 || strcmp(command, "GO") == 0) {
            if (handleMoveCommand(energy, time, room, emergencyTrips, turn, roomCount,
                                  itemTaken, *fuseInstalled, over, victory,
                                  roomId, roomName, roomFloor, linkA, linkB, linkType,
                                  linkCount, tokens, count)) {
                if (!*over) {
                    showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
                               *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
                               *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
                               linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);
                }
            }
        } else if (strcmp(command, "COLLECT") == 0 || strcmp(command, "TAKE") == 0 || strcmp(command, "PICK") == 0) {
            if (handleCollectCommand(energy, time, score, room, turn, *emergencyTrips,
                                     itemTaken, clueFour, clueTwo, clueSeven,
                                     *cabinetUnlocked, over, victory,
                                     roomId, itemCode, itemName, itemSymbol, itemRoom, itemClass,
                                     itemAs, itemValue, itemCount, tokens, count)) {
                if (!*over) {
                    showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
                               *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
                               *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
                               linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);
                }
            }
        } else if (strcmp(command, "USE") == 0) {
            if (handleUseCommand(energy, time, score, room, turn, itemTaken,
                                 clueFour, clueTwo, clueSeven, fuseInstalled, cabinetUnlocked,
                                 passengerRescued, catSecured, trainBoarded, over, victory,
                                 roomCount, roomId, itemCode, itemName, itemSymbol, itemRoom,
                                 itemAs, itemCount, tokens, count)) {
                if (!*over) {
                    showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
                               *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
                               *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
                               linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);
                }
            }
        } else if (strcmp(command, "UNLOCK") == 0) {
            int codeIndex = 1;
            if (count >= 2 && util::equalsIgnoreCase(tokens[1], "CABINET")) {
                codeIndex = 2;
            }
            if (count <= codeIndex) {
                cout << "Usage: UNLOCK CABINET <code>\n";
            } else {
                int code = kNoArgument;
                if (!util::parseIntStrict(tokens[codeIndex], &code)) {
                    cout << "'" << tokens[codeIndex] << "' is not a valid code.\n";
                } else {
                    char message[512];
                    if (switches::tryUse(energy, time, score, room, turn, itemTaken,
                                         clueFour, clueTwo, clueSeven, fuseInstalled, cabinetUnlocked,
                                         passengerRescued, catSecured, trainBoarded, over, victory,
                                         roomId, roomCount, itemName, itemRoom, itemAs, itemCount,
                                         IT_CABINET, code, message, (int)sizeof(message))) {
                        reportAction(*turn, message);
                        if (!*over) {
                            showStatus(*energy, *time, *score, *turn, *room, itemTaken, *fuseInstalled,
                                       *passengerRescued, *catSecured, *clueFour, *clueTwo, *clueSeven,
                                       *cabinetUnlocked, roomId, roomName, roomFloor, linkA, linkB, linkType,
                                       linkCount, itemName, itemSymbol, itemRoom, itemClass, itemAs, itemCount);
                        }
                    } else {
                        cout << message << "\n";
                    }
                }
            }
        } else if (strcmp(command, "BOARD") == 0 || strcmp(command, "PREPARE") == 0) {
            char message[512];
            if (switches::tryUse(energy, time, score, room, turn, itemTaken,
                                 clueFour, clueTwo, clueSeven, fuseInstalled, cabinetUnlocked,
                                 passengerRescued, catSecured, trainBoarded, over, victory,
                                 roomId, roomCount, itemName, itemRoom, itemAs, itemCount,
                                 IT_TRAIN_KEY, kNoArgument, message, (int)sizeof(message))) {
                reportAction(*turn, message);
            } else {
                cout << message << "\n";
            }
        } else if (strcmp(command, "SAVE") == 0) {
            handleSaveCommand(*energy, *time, *score, *room, *turn, *emergencyTrips,
                              *fuseInstalled, *cabinetUnlocked, *passengerRescued,
                              *catSecured, *trainBoarded, itemTaken, roomId, itemCode,
                              itemAs, itemCount);
        } else if (strcmp(command, "LOAD") == 0) {
            handleLoadCommand(energy, time, score, room, turn, emergencyTrips,
                              fuseInstalled, cabinetUnlocked, passengerRescued,
                              catSecured, trainBoarded, clueFour, clueTwo, clueSeven,
                              itemTaken, over, victory, roomId, roomCount, roomName, roomFloor,
                              linkA, linkB, linkType, linkCount, itemCode, itemSymbol,
                              itemName, itemRoom, itemClass, itemAs, itemCount);
        } else if (strcmp(command, "QUIT") == 0 || strcmp(command, "EXIT") == 0) {
            quit = true;
        } else {
            cout << "Unknown command '" << command << "'. Type HELP.\n";
        }
    }

    const char* result = "INCOMPLETE";
    if (*victory) result = "VICTORY";
    else if (*over) result = "DEFEAT";

    if (*over) {
        viewer::printResult(*energy, *time, *score, *turn, *victory);
    } else {
        cout << "\nSession ended (" << result << ").\n";
    }

    char error[256];
    if (!logger::writeGameSummaryReport(*energy, *time, *score, *room, *turn,
                                        *fuseInstalled, *cabinetUnlocked, *passengerRescued,
                                        *catSecured, *trainBoarded, *victory,
                                        *clueFour, *clueTwo, *clueSeven, itemTaken,
                                        roomId, roomName, result, cfg::kPathSummary,
                                        error, (int)sizeof(error))) {
        cerr << "Summary write failed: " << error << "\n";
    } else {
        cout << "Session summary written to " << cfg::kPathSummary << "\n";
    }

    if (*victory) return 0;
    return *over ? 1 : 2;
}