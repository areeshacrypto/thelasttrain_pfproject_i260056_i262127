#include "logger.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "util.h"

namespace {

char g_audit[cfg::kMaxAuditLines][cfg::kAuditLineLen];
int g_auditCount = 0;

bool readDataLine(std::FILE* file, char* buffer, int cap) {
    while (std::fgets(buffer, cap, file) != nullptr) {
        util::trimInPlace(buffer);
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        return true;
    }
    return false;
}

void writeObjective(std::FILE* file, bool done, const char* label, int points) {
    std::fprintf(file, "[%c] %-44s +%d\n", done ? 'x' : ' ', label, done ? points : 0);
}

bool parseRange(const char* value, int minValue, int maxValue, int* out) {
    int parsed = 0;
    if (!util::parseIntStrict(value, &parsed) || parsed < minValue || parsed > maxValue) {
        return false;
    }
    *out = parsed;
    return true;
}

}  // namespace

void logger::resetAudit() {
    g_auditCount = 0;
}

void logger::addAudit(const char* format, ...) {
    if (format == nullptr || g_auditCount >= cfg::kMaxAuditLines) {
        return;
    }
    va_list args;
    va_start(args, format);
    std::vsnprintf(g_audit[g_auditCount], cfg::kAuditLineLen, format, args);
    va_end(args);
    ++g_auditCount;
}

int logger::auditCount() {
    return g_auditCount;
}

const char* logger::auditLine(int index) {
    if (index < 0 || index >= g_auditCount) {
        return "";
    }
    return g_audit[index];
}

bool logger::saveGameStateToFile(const GameState* state, const BuildingMap* map,
                                 const char* path, char* error, int errorCap) {
    std::FILE* file = std::fopen(path, "w");
    if (file == nullptr) {
        std::snprintf(error, static_cast<size_t>(errorCap), "cannot open %s for writing",
                      path);
        return false;
    }
    std::fprintf(file, "VERSION 1\n");
    std::fprintf(file, "TURN %d\n", state->turn);
    std::fprintf(file, "ROOM %s\n", map->rooms[state->room].id);
    std::fprintf(file, "ENERGY %d\n", state->energy);
    std::fprintf(file, "TIME %d\n", state->time);
    std::fprintf(file, "SCORE %d\n", state->score);
    std::fprintf(file, "TRIPS %d\n", state->emergencyTrips);
    std::fprintf(file, "FUSE_INSTALLED %d\n", state->fuseInstalled ? 1 : 0);
    std::fprintf(file, "CABINET_OPEN %d\n", state->cabinetUnlocked ? 1 : 0);
    std::fprintf(file, "PASSENGER_RESCUED %d\n", state->passengerRescued ? 1 : 0);
    std::fprintf(file, "CAT_SECURED %d\n", state->catSecured ? 1 : 0);
    std::fprintf(file, "TRAIN_BOARDED %d\n", state->trainBoarded ? 1 : 0);
    std::fprintf(file, "TAKEN");
    bool any = false;
    for (int i = 0; i < map->itemCount; ++i) {
        const ItemDef& def = map->items[i];
        if (state->itemTaken[def.itemId]) {
            std::fprintf(file, "%s%s", any ? "," : " ", def.id);
            any = true;
        }
    }
    if (!any) {
        std::fprintf(file, " -");
    }
    std::fprintf(file, "\n");
    if (std::fclose(file) != 0) {
        std::snprintf(error, static_cast<size_t>(errorCap), "cannot finish writing %s",
                      path);
        return false;
    }
    return true;
}

bool logger::loadGameStateFromFile(GameState* state, const BuildingMap* map,
                                   const char* path, char* error, int errorCap) {
    std::FILE* file = std::fopen(path, "r");
    if (file == nullptr) {
        std::snprintf(error, static_cast<size_t>(errorCap), "cannot open %s for reading",
                      path);
        return false;
    }

    GameState temp{};
    temp.room = -1;
    temp.emergencyTrips = cfg::kStartEmergencyTrips;

    bool seenRoom = false;
    bool seenEnergy = false;
    bool seenTime = false;
    bool seenScore = false;
    bool seenTurn = false;
    bool seenTrips = false;
    bool seenFuse = false;
    bool seenCabinet = false;
    bool seenPassenger = false;
    bool seenCat = false;
    bool seenBoarded = false;
    bool seenTaken = false;
    bool ok = true;
    char reason[96] = "malformed state file";

    char line[cfg::kMaxLineLen];
    int lineNumber = 0;
    while (ok && readDataLine(file, line, sizeof line)) {
        ++lineNumber;
        char* value = line;
        while (*value != '\0' && *value != ' ') {
            ++value;
        }
        if (*value == ' ') {
            *value = '\0';
            ++value;
            util::trimInPlace(value);
        }

        const char* key = line;
        char* val = value;

        if (std::strcmp(key, "VERSION") == 0) {
            if (std::strcmp(val, "1") != 0) {
                ok = false;
                std::snprintf(reason, sizeof reason, "unsupported VERSION on line %d",
                              lineNumber);
            }
        } else if (std::strcmp(key, "TURN") == 0) {
            ok = parseRange(val, 0, 100000, &temp.turn);
            seenTurn = ok;
        } else if (std::strcmp(key, "ROOM") == 0) {
            const int room = findRoom(map, val);
            if (room < 0) {
                ok = false;
                std::snprintf(reason, sizeof reason, "unknown ROOM on line %d", lineNumber);
            } else {
                temp.room = room;
                seenRoom = true;
            }
        } else if (std::strcmp(key, "ENERGY") == 0) {
            ok = parseRange(val, 0, cfg::kResourceCap, &temp.energy);
            seenEnergy = ok;
        } else if (std::strcmp(key, "TIME") == 0) {
            ok = parseRange(val, 0, cfg::kResourceCap, &temp.time);
            seenTime = ok;
        } else if (std::strcmp(key, "SCORE") == 0) {
            ok = parseRange(val, 0, cfg::kMaxScore, &temp.score);
            seenScore = ok;
        } else if (std::strcmp(key, "TRIPS") == 0) {
            ok = parseRange(val, 0, cfg::kStartEmergencyTrips, &temp.emergencyTrips);
            seenTrips = ok;
        } else if (std::strcmp(key, "FUSE_INSTALLED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.fuseInstalled = flag != 0;
            seenFuse = ok;
        } else if (std::strcmp(key, "CABINET_OPEN") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.cabinetUnlocked = flag != 0;
            seenCabinet = ok;
        } else if (std::strcmp(key, "PASSENGER_RESCUED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.passengerRescued = flag != 0;
            seenPassenger = ok;
        } else if (std::strcmp(key, "CAT_SECURED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.catSecured = flag != 0;
            seenCat = ok;
        } else if (std::strcmp(key, "TRAIN_BOARDED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.trainBoarded = flag != 0;
            seenBoarded = ok;
        } else if (std::strcmp(key, "TAKEN") == 0) {
            seenTaken = true;
            char* cursor = val;
            while (ok && *cursor != '\0') {
                char* comma = std::strchr(cursor, ',');
                if (comma != nullptr) {
                    *comma = '\0';
                }
                util::trimInPlace(cursor);
                if (*cursor != '\0' && std::strcmp(cursor, "-") != 0) {
                    const int index = findItemDef(map, cursor);
                    if (index < 0) {
                        ok = false;
                        std::snprintf(reason, sizeof reason,
                                      "unknown item '%s' on line %d", cursor, lineNumber);
                    } else {
                        temp.itemTaken[map->items[index].itemId] = true;
                    }
                }
                if (comma == nullptr) {
                    break;
                }
                cursor = comma + 1;
            }
        } else {
            ok = false;
            std::snprintf(reason, sizeof reason, "unknown key '%s' on line %d", key,
                          lineNumber);
        }
    }
    std::fclose(file);

    if (ok &&
        !(seenRoom && seenEnergy && seenTime && seenScore && seenTurn && seenTrips &&
          seenFuse && seenCabinet && seenPassenger && seenCat && seenBoarded && seenTaken)) {
        ok = false;
        std::snprintf(reason, sizeof reason, "state file is missing required keys");
    }
    if (!ok) {
        std::snprintf(error, static_cast<size_t>(errorCap), "%s: %s", path, reason);
        return false;
    }

    temp.clueFour = temp.itemTaken[IT_CLUE_FOUR];
    temp.clueTwo = temp.itemTaken[IT_CLUE_TWO];
    temp.clueSeven = temp.itemTaken[IT_CLUE_SEVEN];
    if (temp.trainBoarded) {
        temp.over = true;
        temp.victory = true;
    } else if (temp.energy <= 0 || temp.time <= 0) {
        temp.over = true;
        temp.victory = false;
    }

    *state = temp;
    return true;
}

bool logger::writeGameSummaryReport(const GameState* state, const BuildingMap* map,
                                    const char* result, const char* path, char* error,
                                    int errorCap) {
    std::FILE* file = std::fopen(path, "w");
    if (file == nullptr) {
        std::snprintf(error, static_cast<size_t>(errorCap), "cannot open %s for writing",
                      path);
        return false;
    }
    char clues[16];
    formatClueCode(state, clues, static_cast<int>(sizeof clues));

    std::fprintf(file, "THE LAST TRAIN - SESSION SUMMARY\n");
    std::fprintf(file, "================================\n");
    std::fprintf(file, "Result       : %s\n", result);
    std::fprintf(file, "Turn count   : %d\n", state->turn);
    std::fprintf(file, "Final room   : %s (%s)\n", map->rooms[state->room].id,
                 map->rooms[state->room].name);
    std::fprintf(file, "Energy       : %d / %d\n", state->energy, cfg::kResourceCap);
    std::fprintf(file, "Time         : %d / %d\n", state->time, cfg::kResourceCap);
    std::fprintf(file, "Final score  : %d / %d\n", state->score, cfg::kMaxScore);
    std::fprintf(file, "Cabinet code : %s (%s)\n", clues,
                 state->cabinetUnlocked ? "unlocked" : "locked");

    std::fprintf(file, "\nOBJECTIVE CHECKLIST\n");
    std::fprintf(file, "-------------------\n");
    writeObjective(file, state->itemTaken[IT_MASTER_KEY], "Master Key collected",
                   cfg::kScoreMasterKey);
    writeObjective(file, state->clueFour, "Clue digit 4 found", cfg::kScoreClue);
    writeObjective(file, state->clueTwo, "Clue digit 2 found", cfg::kScoreClue);
    writeObjective(file, state->clueSeven, "Clue digit 7 found", cfg::kScoreClue);
    writeObjective(file, state->cabinetUnlocked, "Security cabinet unlocked (427)",
                   cfg::kScoreCabinet);
    writeObjective(file, state->itemTaken[IT_TRAIN_KEY], "Train Key acquired",
                   cfg::kScoreTrainKey);
    writeObjective(file, state->passengerRescued, "Injured passenger rescued",
                   cfg::kScorePassenger);
    writeObjective(file, state->catSecured, "Cat secured in carrier", cfg::kScoreCat);
    writeObjective(file, state->fuseInstalled, "Fuse installed at Control Panel",
                   cfg::kScoreFuse);
    writeObjective(file, state->itemTaken[IT_LIFT_PASS], "Lift Pass collected",
                   cfg::kScoreLiftPass);
    writeObjective(file, state->trainBoarded, "Train prepared and boarded", cfg::kScoreTrain);
    writeObjective(file, state->victory, "Full mission victory bonus",
                   cfg::kScoreVictoryBonus);

    std::fprintf(file, "\nAUDIT TRAIL\n");
    std::fprintf(file, "-----------\n");
    if (g_auditCount == 0) {
        std::fprintf(file, "(no actions recorded)\n");
    }
    for (int i = 0; i < g_auditCount; ++i) {
        std::fprintf(file, "%02d. %s\n", i + 1, g_audit[i]);
    }
    std::fprintf(file, "\nEND OF REPORT\n");

    if (std::fclose(file) != 0) {
        std::snprintf(error, static_cast<size_t>(errorCap), "cannot finish writing %s",
                      path);
        return false;
    }
    return true;
}
