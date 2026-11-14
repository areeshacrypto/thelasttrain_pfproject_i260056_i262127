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

static void reportAction(GameState* state, const char* message) {
    cout << message << "\n";
    char entry[cfg::kAuditLineLen] = "Turn ";
    util::appendInt(entry, cfg::kAuditLineLen, state->turn);
    util::appendStr(entry, cfg::kAuditLineLen, ": ");
    util::appendStr(entry, cfg::kAuditLineLen, message);
    logger::addAudit(entry);
}

static void handleInspectCommand(GameState* state, const BuildingMap* map) {
    applyCost(state, costInspect());
    state->turn++;
    clampResources(state);

    const Room& room = map->rooms[state->room];
    cout << "You inspect " << room.id << " (" << room.name << ").\n";

    bool found = false;
    for (int i = 0; i < map->itemCount; i++) {
        const ItemDef& def = map->items[i];
        if (def.room != state->room || state->itemTaken[def.itemId]) continue;
        if (def.itemId == IT_TRAIN_KEY && !state->cabinetUnlocked) continue;
        cout << "  - " << def.symbol << " " << def.name << " (" << itemClassName(def.cls) << ")\n";
        found = true;
    }
    if (!found) {
        cout << "  Nothing else of interest here.\n";
    }

    for (int i = 0; i < map->itemCount; i++) {
        const ItemDef& def = map->items[i];
        if (def.room != state->room) continue;
        switch (def.itemId) {
            case IT_PANEL:
                cout << "  The control panel is " << (state->fuseInstalled ? "powered" : "dark; it needs a Fuse") << ".\n";
                break;
            case IT_CABINET:
                cout << "  The security cabinet is " << (state->cabinetUnlocked ? "open" : "locked by a 3-digit keypad") << ".\n";
                break;
            case IT_CAT:
                cout << "  The cat is " << (state->catSecured ? "safe in the carrier" : "watching you") << ".\n";
                break;
            case IT_PASSENGER:
                cout << "  The injured passenger is " << (state->passengerRescued ? "ready to move" : "waiting for help") << ".\n";
                break;
            default:
                break;
        }
    }

    char entry[cfg::kAuditLineLen] = "Turn ";
    util::appendInt(entry, cfg::kAuditLineLen, state->turn);
    util::appendStr(entry, cfg::kAuditLineLen, ": inspected ");
    util::appendStr(entry, cfg::kAuditLineLen, room.id);
    logger::addAudit(entry);
    resolveDefeat(state);
}

static void handleMoveCommand(GameState* state, const BuildingMap* map, char* tokens[], int count) {
    if (count < 2) {
        cout << "Usage: MOVE <room>   (e.g. MOVE F3-R2)\n";
        return;
    }
    int target = findRoom(map, tokens[1]);
    if (target < 0) {
        cout << "Unknown room '" << tokens[1] << "'. Type MAP to see the building.\n";
        return;
    }
    char message[512];
    if (switches::tryTraverse(state, map, target, message, (int)sizeof(message))) {
        reportAction(state, message);
        if (!state->over) viewer::printStatus(map, state);
    } else {
        cout << message << "\n";
    }
}

static void handleCollectCommand(GameState* state, const BuildingMap* map, char* tokens[], int count) {
    if (count < 2) {
        cout << "Usage: COLLECT <item>   (id, name, or symbol)\n";
        return;
    }
    char message[512];
    if (switches::tryCollect(state, map, tokens[1], message, (int)sizeof(message))) {
        reportAction(state, message);
        if (!state->over) viewer::printStatus(map, state);
    } else {
        cout << message << "\n";
    }
}

static void handleUseCommand(GameState* state, const BuildingMap* map, char* tokens[], int count) {
    if (count < 2) {
        cout << "Usage: USE <item> [argument]\n";
        return;
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
        int index = findItemDef(map, target);
        if (index < 0) {
            cout << "Unknown item '" << target << "'.\n";
            return;
        }
        id = map->items[index].itemId;
    }

    int argument = kNoArgument;
    if (count >= 3 && !util::parseIntStrict(tokens[2], &argument)) {
        cout << "'" << tokens[2] << "' is not a valid code.\n";
        return;
    }
    if (id == IT_CABINET && count < 3) {
        cout << "Usage: UNLOCK CABINET <code>\n";
        return;
    }

    char message[512];
    if (switches::tryUse(state, map, id, argument, message, (int)sizeof(message))) {
        reportAction(state, message);
        if (!state->over) viewer::printStatus(map, state);
    } else {
        cout << message << "\n";
    }
}

static void handleSaveCommand(const GameState* state, const BuildingMap* map) {
    char error[256];
    if (logger::saveGameStateToFile(state, map, cfg::kPathSaveGame, error, (int)sizeof(error))) {
        cout << "Game saved to " << cfg::kPathSaveGame << "\n";
    } else {
        cout << "Save failed: " << error << "\n";
    }
}

static void handleLoadCommand(GameState* state, const BuildingMap* map) {
    char error[256];
    if (logger::loadGameStateFromFile(state, map, cfg::kPathSaveGame, error, (int)sizeof(error))) {
        cout << "Game loaded from " << cfg::kPathSaveGame << "\n";
        viewer::printStatus(map, state);
    } else {
        cout << "Load failed: " << error << "\n";
    }
}

int simulator::runSimulation(const BuildingMap* map, GameState* state, FILE* input, bool echoCommands) {
    cout << "\nType HELP for the command list.\n";
    viewer::printStatus(map, state);

    bool quit = false;
    char line[cfg::kMaxLineLen];

    while (!state->over && !quit) {
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
            viewer::printStatus(map, state);
        } else if (strcmp(command, "MAP") == 0) {
            viewer::printMapOverview(map, state);
        } else if (strcmp(command, "INSPECT") == 0 || strcmp(command, "SEARCH") == 0) {
            handleInspectCommand(state, map);
            if (!state->over) viewer::printStatus(map, state);
        } else if (strcmp(command, "MOVE") == 0 || strcmp(command, "GO") == 0) {
            handleMoveCommand(state, map, tokens, count);
        } else if (strcmp(command, "COLLECT") == 0 || strcmp(command, "TAKE") == 0 || strcmp(command, "PICK") == 0) {
            handleCollectCommand(state, map, tokens, count);
        } else if (strcmp(command, "USE") == 0) {
            handleUseCommand(state, map, tokens, count);
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
                    if (switches::tryUse(state, map, IT_CABINET, code, message, (int)sizeof(message))) {
                        reportAction(state, message);
                        if (!state->over) viewer::printStatus(map, state);
                    } else {
                        cout << message << "\n";
                    }
                }
            }
        } else if (strcmp(command, "BOARD") == 0 || strcmp(command, "PREPARE") == 0) {
            char message[512];
            if (switches::tryUse(state, map, IT_TRAIN_KEY, kNoArgument, message, (int)sizeof(message))) {
                reportAction(state, message);
            } else {
                cout << message << "\n";
            }
        } else if (strcmp(command, "SAVE") == 0) {
            handleSaveCommand(state, map);
        } else if (strcmp(command, "LOAD") == 0) {
            handleLoadCommand(state, map);
        } else if (strcmp(command, "QUIT") == 0 || strcmp(command, "EXIT") == 0) {
            quit = true;
        } else {
            cout << "Unknown command '" << command << "'. Type HELP.\n";
        }
    }

    const char* result = "INCOMPLETE";
    if (state->victory) result = "VICTORY";
    else if (state->over) result = "DEFEAT";

    if (state->over) {
        viewer::printResult(map, state);
    } else {
        cout << "\nSession ended (" << result << ").\n";
    }

    char error[256];
    if (!logger::writeGameSummaryReport(state, map, result, cfg::kPathSummary, error, (int)sizeof(error))) {
        cerr << "Summary write failed: " << error << "\n";
    } else {
        cout << "Session summary written to " << cfg::kPathSummary << "\n";
    }

    if (state->victory) return 0;
    return state->over ? 1 : 2;
}