#include "simulator.h"

#include <cstring>

#include "config.h"
#include "level.h"
#include "logger.h"
#include "physics.h"
#include "state.h"
#include "switches.h"
#include "util.h"
#include "viewer_iface.h"

namespace {

const int kNoArgument = -1000000;

void reportAction(GameState* state, const char* message) {
    std::printf("%s\n", message);
    logger::addAudit("Turn %d: %s", state->turn, message);
}

void handleInspectCommand(GameState* state, const BuildingMap* map) {
    applyCost(state, costInspect());
    ++state->turn;
    clampResources(state);

    const Room& room = map->rooms[state->room];
    std::printf("You inspect %s (%s).\n", room.id, room.name);

    bool found = false;
    for (int i = 0; i < map->itemCount; ++i) {
        const ItemDef& def = map->items[i];
        if (def.room != state->room || state->itemTaken[def.itemId]) {
            continue;
        }
        if (def.itemId == IT_TRAIN_KEY && !state->cabinetUnlocked) {
            continue;
        }
        std::printf("  - %c %s (%s)\n", def.symbol, def.name, itemClassName(def.cls));
        found = true;
    }
    if (!found) {
        std::printf("  Nothing else of interest here.\n");
    }

    for (int i = 0; i < map->itemCount; ++i) {
        const ItemDef& def = map->items[i];
        if (def.room != state->room) {
            continue;
        }
        switch (def.itemId) {
            case IT_PANEL:
                std::printf("  The control panel is %s.\n",
                            state->fuseInstalled ? "powered" : "dark; it needs a Fuse");
                break;
            case IT_CABINET:
                std::printf("  The security cabinet is %s.\n",
                            state->cabinetUnlocked ? "open" : "locked by a 3-digit keypad");
                break;
            case IT_CAT:
                std::printf("  The cat is %s.\n",
                            state->catSecured ? "safe in the carrier" : "watching you");
                break;
            case IT_PASSENGER:
                std::printf("  The injured passenger is %s.\n",
                            state->passengerRescued ? "ready to move" : "waiting for help");
                break;
            default:
                break;
        }
    }

    logger::addAudit("Turn %d: inspected %s", state->turn, room.id);
    resolveDefeat(state);
}

void handleMoveCommand(GameState* state, const BuildingMap* map, char* tokens[],
                       int count) {
    if (count < 2) {
        std::printf("Usage: MOVE <room>   (e.g. MOVE F3-R2)\n");
        return;
    }
    const int target = findRoom(map, tokens[1]);
    if (target < 0) {
        std::printf("Unknown room '%s'. Type MAP to see the building.\n", tokens[1]);
        return;
    }
    char message[512];
    if (switches::tryTraverse(state, map, target, message,
                              static_cast<int>(sizeof message))) {
        reportAction(state, message);
        if (!state->over) {
            viewer::printStatus(map, state);
        }
    } else {
        std::printf("%s\n", message);
    }
}

void handleCollectCommand(GameState* state, const BuildingMap* map, char* tokens[],
                          int count) {
    if (count < 2) {
        std::printf("Usage: COLLECT <item>   (id, name, or symbol)\n");
        return;
    }
    char message[512];
    if (switches::tryCollect(state, map, tokens[1], message,
                             static_cast<int>(sizeof message))) {
        reportAction(state, message);
        if (!state->over) {
            viewer::printStatus(map, state);
        }
    } else {
        std::printf("%s\n", message);
    }
}

void handleUseCommand(GameState* state, const BuildingMap* map, char* tokens[],
                      int count) {
    if (count < 2) {
        std::printf("Usage: USE <item> [argument]\n");
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
        const int index = findItemDef(map, target);
        if (index < 0) {
            std::printf("Unknown item '%s'.\n", target);
            return;
        }
        id = map->items[index].itemId;
    }

    int argument = kNoArgument;
    if (count >= 3 && !util::parseIntStrict(tokens[2], &argument)) {
        std::printf("'%s' is not a valid code.\n", tokens[2]);
        return;
    }
    if (id == IT_CABINET && count < 3) {
        std::printf("Usage: UNLOCK CABINET <code>\n");
        return;
    }

    char message[512];
    if (switches::tryUse(state, map, id, argument, message,
                         static_cast<int>(sizeof message))) {
        reportAction(state, message);
        if (!state->over) {
            viewer::printStatus(map, state);
        }
    } else {
        std::printf("%s\n", message);
    }
}

void handleSaveCommand(const GameState* state, const BuildingMap* map) {
    char error[256];
    if (logger::saveGameStateToFile(state, map, cfg::kPathSaveGame, error,
                                    static_cast<int>(sizeof error))) {
        std::printf("Game saved to %s\n", cfg::kPathSaveGame);
    } else {
        std::printf("Save failed: %s\n", error);
    }
}

void handleLoadCommand(GameState* state, const BuildingMap* map) {
    char error[256];
    if (logger::loadGameStateFromFile(state, map, cfg::kPathSaveGame, error,
                                      static_cast<int>(sizeof error))) {
        std::printf("Game loaded from %s\n", cfg::kPathSaveGame);
        viewer::printStatus(map, state);
    } else {
        std::printf("Load failed: %s\n", error);
    }
}

}  // namespace

int simulator::runSimulation(const BuildingMap* map, GameState* state,
                             std::FILE* input, bool echoCommands) {
    std::printf("\n%s\n", "Type HELP for the command list.");
    viewer::printStatus(map, state);

    bool quit = false;
    char line[cfg::kMaxLineLen];

    while (!state->over && !quit) {
        std::printf("> ");
        std::fflush(stdout);
        if (std::fgets(line, sizeof line, input) == nullptr) {
            break;
        }
        if (echoCommands) {
            std::printf("%s", line);
            const size_t len = std::strlen(line);
            if (len > 0 && line[len - 1] != '\n') {
                std::printf("\n");
            }
            std::fflush(stdout);
        }

        util::trimInPlace(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        char* tokens[cfg::kMaxTokens] = {};
        const int count = util::splitTokens(line, tokens, cfg::kMaxTokens);
        if (count == 0) {
            continue;
        }
        util::toUpperInPlace(tokens[0]);
        const char* command = tokens[0];

        if (std::strcmp(command, "HELP") == 0 || std::strcmp(command, "H") == 0 ||
            std::strcmp(command, "?") == 0) {
            viewer::printHelp();
        } else if (std::strcmp(command, "LOOK") == 0 ||
                   std::strcmp(command, "L") == 0 ||
                   std::strcmp(command, "STATUS") == 0) {
            viewer::printStatus(map, state);
        } else if (std::strcmp(command, "MAP") == 0) {
            viewer::printMapOverview(map, state);
        } else if (std::strcmp(command, "INSPECT") == 0 ||
                   std::strcmp(command, "SEARCH") == 0) {
            handleInspectCommand(state, map);
            if (!state->over) {
                viewer::printStatus(map, state);
            }
        } else if (std::strcmp(command, "MOVE") == 0 ||
                   std::strcmp(command, "GO") == 0) {
            handleMoveCommand(state, map, tokens, count);
        } else if (std::strcmp(command, "COLLECT") == 0 ||
                   std::strcmp(command, "TAKE") == 0 ||
                   std::strcmp(command, "PICK") == 0) {
            handleCollectCommand(state, map, tokens, count);
        } else if (std::strcmp(command, "USE") == 0) {
            handleUseCommand(state, map, tokens, count);
        } else if (std::strcmp(command, "UNLOCK") == 0) {
            int codeIndex = 1;
            if (count >= 2 && util::equalsIgnoreCase(tokens[1], "CABINET")) {
                codeIndex = 2;
            }
            if (count <= codeIndex) {
                std::printf("Usage: UNLOCK CABINET <code>\n");
            } else {
                int code = kNoArgument;
                if (!util::parseIntStrict(tokens[codeIndex], &code)) {
                    std::printf("'%s' is not a valid code.\n", tokens[codeIndex]);
                } else {
                    char message[512];
                    if (switches::tryUse(state, map, IT_CABINET, code, message,
                                         static_cast<int>(sizeof message))) {
                        reportAction(state, message);
                        if (!state->over) {
                            viewer::printStatus(map, state);
                        }
                    } else {
                        std::printf("%s\n", message);
                    }
                }
            }
        } else if (std::strcmp(command, "BOARD") == 0 ||
                   std::strcmp(command, "PREPARE") == 0) {
            char message[512];
            if (switches::tryUse(state, map, IT_TRAIN_KEY, kNoArgument, message,
                                 static_cast<int>(sizeof message))) {
                reportAction(state, message);
            } else {
                std::printf("%s\n", message);
            }
        } else if (std::strcmp(command, "SAVE") == 0) {
            handleSaveCommand(state, map);
        } else if (std::strcmp(command, "LOAD") == 0) {
            handleLoadCommand(state, map);
        } else if (std::strcmp(command, "QUIT") == 0 ||
                   std::strcmp(command, "EXIT") == 0) {
            quit = true;
        } else {
            std::printf("Unknown command '%s'. Type HELP.\n", command);
        }
    }

    const char* result = "INCOMPLETE";
    if (state->victory) {
        result = "VICTORY";
    } else if (state->over) {
        result = "DEFEAT";
    }

    if (state->over) {
        viewer::printResult(map, state);
    } else {
        std::printf("\nSession ended (%s).\n", result);
    }

    char error[256];
    if (!logger::writeGameSummaryReport(state, map, result, cfg::kPathSummary, error,
                                        static_cast<int>(sizeof error))) {
        std::fprintf(stderr, "Summary write failed: %s\n", error);
    } else {
        std::printf("Session summary written to %s\n", cfg::kPathSummary);
    }

    if (state->victory) {
        return 0;
    }
    return state->over ? 1 : 2;
}
