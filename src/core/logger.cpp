#include "logger.h"
#include <cstring>
#include <fstream>
#include <iomanip>
#include "util.h"

using namespace std;

static char g_audit[cfg::kMaxAuditLines][cfg::kAuditLineLen];
static int g_auditCount = 0;

static bool readDataLine(ifstream& file, char* buffer, int cap) {
    while (file.getline(buffer, cap)) {
        util::trimInPlace(buffer);
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        return true;
    }
    return false;
}

static void writeObjective(ofstream& file, bool done, const char* label, int points) {
    file << "[" << (done ? 'x' : ' ') << "] "
         << left << setw(44) << label
         << " +" << (done ? points : 0) << "\n";
}

static bool parseRange(const char* value, int minValue, int maxValue, int* out) {
    int parsed = 0;
    if (!util::parseIntStrict(value, &parsed) || parsed < minValue || parsed > maxValue) {
        return false;
    }
    *out = parsed;
    return true;
}

void logger::resetAudit() {
    g_auditCount = 0;
}

void logger::addAudit(const char* text) {
    if (text == nullptr || g_auditCount >= cfg::kMaxAuditLines) return;
    util::copyText(g_audit[g_auditCount], cfg::kAuditLineLen, text);
    g_auditCount++;
}

int logger::auditCount() {
    return g_auditCount;
}

const char* logger::auditLine(int index) {
    if (index < 0 || index >= g_auditCount) return "";
    return g_audit[index];
}

bool logger::saveGameStateToFile(const GameState* state, const BuildingMap* map,
                                 const char* path, char* error, int errorCap) {
    ofstream file(path);
    if (!file.is_open()) {
        util::copyText(error, errorCap, "Cannot open file for writing: ");
        util::appendStr(error, errorCap, path);
        return false;
    }

    file << "VERSION 1\n";
    file << "TURN " << state->turn << "\n";
    file << "ROOM " << map->rooms[state->room].id << "\n";
    file << "ENERGY " << state->energy << "\n";
    file << "TIME " << state->time << "\n";
    file << "SCORE " << state->score << "\n";
    file << "TRIPS " << state->emergencyTrips << "\n";
    file << "FUSE_INSTALLED " << (state->fuseInstalled ? 1 : 0) << "\n";
    file << "CABINET_OPEN " << (state->cabinetUnlocked ? 1 : 0) << "\n";
    file << "PASSENGER_RESCUED " << (state->passengerRescued ? 1 : 0) << "\n";
    file << "CAT_SECURED " << (state->catSecured ? 1 : 0) << "\n";
    file << "TRAIN_BOARDED " << (state->trainBoarded ? 1 : 0) << "\n";

    file << "TAKEN";
    bool any = false;
    for (int i = 0; i < map->itemCount; i++) {
        const ItemDef& def = map->items[i];
        if (state->itemTaken[def.itemId]) {
            file << (any ? "," : " ") << def.id;
            any = true;
        }
    }
    if (!any) {
        file << " -";
    }
    file << "\n";

    if (file.fail()) {
        util::copyText(error, errorCap, "Error while writing: ");
        util::appendStr(error, errorCap, path);
        return false;
    }
    file.close();
    return true;
}

bool logger::loadGameStateFromFile(GameState* state, const BuildingMap* map,
                                   const char* path, char* error, int errorCap) {
    ifstream file(path);
    if (!file.is_open()) {
        util::copyText(error, errorCap, "Cannot open file for reading: ");
        util::appendStr(error, errorCap, path);
        return false;
    }

    GameState temp = {};
    temp.room = -1;
    temp.emergencyTrips = cfg::kStartEmergencyTrips;
    bool seenRoom = false, seenEnergy = false, seenTime = false;
    bool seenScore = false, seenTurn = false, seenTrips = false;
    bool seenFuse = false, seenCabinet = false, seenPassenger = false;
    bool seenCat = false, seenBoarded = false, seenTaken = false;
    bool ok = true;
    char reason[96] = "malformed state file";
    char line[cfg::kMaxLineLen];
    while (ok && readDataLine(file, line, sizeof(line))) {
        char* value = line;
        while (*value != '\0' && *value != ' ') {
            value++;
        }
        if (*value == ' ') {
            *value = '\0';
            value++;
            util::trimInPlace(value);
        }

        const char* key = line;
        char* val = value;

        if (strcmp(key, "VERSION") == 0) {
            if (strcmp(val, "1") != 0) {
                ok = false;
                util::copyText(reason, sizeof(reason), "unsupported VERSION");
            }
        } else if (strcmp(key, "TURN") == 0) {
            ok = parseRange(val, 0, 100000, &temp.turn);
            seenTurn = ok;
        } else if (strcmp(key, "ROOM") == 0) {
            int room = findRoom(map, val);
            if (room < 0) {
                ok = false;
                util::copyText(reason, sizeof(reason), "unknown ROOM id");
            } else {
                temp.room = room;
                seenRoom = true;
            }
        } else if (strcmp(key, "ENERGY") == 0) {
            ok = parseRange(val, 0, cfg::kResourceCap, &temp.energy);
            seenEnergy = ok;
        } else if (strcmp(key, "TIME") == 0) {
            ok = parseRange(val, 0, cfg::kResourceCap, &temp.time);
            seenTime = ok;
        } else if (strcmp(key, "SCORE") == 0) {
            ok = parseRange(val, 0, cfg::kMaxScore, &temp.score);
            seenScore = ok;
        } else if (strcmp(key, "TRIPS") == 0) {
            ok = parseRange(val, 0, cfg::kStartEmergencyTrips, &temp.emergencyTrips);
            seenTrips = ok;
        } else if (strcmp(key, "FUSE_INSTALLED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.fuseInstalled = (flag != 0);
            seenFuse = ok;
        } else if (strcmp(key, "CABINET_OPEN") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.cabinetUnlocked = (flag != 0);
            seenCabinet = ok;
        } else if (strcmp(key, "PASSENGER_RESCUED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.passengerRescued = (flag != 0);
            seenPassenger = ok;
        } else if (strcmp(key, "CAT_SECURED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.catSecured = (flag != 0);
            seenCat = ok;
        } else if (strcmp(key, "TRAIN_BOARDED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            temp.trainBoarded = (flag != 0);
            seenBoarded = ok;
        } else if (strcmp(key, "TAKEN") == 0) {
            seenTaken = true;
            char* cursor = val;
            while (ok && *cursor != '\0') {
                char* comma = strchr(cursor, ',');
                if (comma != nullptr) *comma = '\0';
                util::trimInPlace(cursor);

                if (*cursor != '\0' && strcmp(cursor, "-") != 0) {
                    int index = findItemDef(map, cursor);
                    if (index < 0) {
                        ok = false;
                        util::copyText(reason, sizeof(reason), "unknown item in TAKEN");
                    } else {
                        temp.itemTaken[map->items[index].itemId] = true;
                    }
                }
                if (comma == nullptr) break;
                cursor = comma + 1;
            }
        } else {
            ok = false;
            util::copyText(reason, sizeof(reason), "unknown key in state file");
        }
    }
    file.close();

    if (ok && !(seenRoom && seenEnergy && seenTime && seenScore && seenTurn && seenTrips &&
                seenFuse && seenCabinet && seenPassenger && seenCat && seenBoarded && seenTaken)) {
        ok = false;
        util::copyText(reason, sizeof(reason), "missing required keys");
    }

    if (!ok) {
        util::copyText(error, errorCap, path);
        util::appendStr(error, errorCap, ": ");
        util::appendStr(error, errorCap, reason);
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
    ofstream file(path);
    if (!file.is_open()) {
        util::copyText(error, errorCap, "Cannot open file for writing: ");
        util::appendStr(error, errorCap, path);
        return false;
    }

    char clues[16];
    formatClueCode(state, clues, (int)sizeof(clues));

    file << "THE LAST TRAIN - SESSION SUMMARY\n";
    file << "================================\n";
    file << "Result       : " << result << "\n";
    file << "Turn count   : " << state->turn << "\n";
    file << "Final room   : " << map->rooms[state->room].id << " (" << map->rooms[state->room].name << ")\n";
    file << "Energy       : " << state->energy << " / " << cfg::kResourceCap << "\n";
    file << "Time         : " << state->time << " / " << cfg::kResourceCap << "\n";
    file << "Final score  : " << state->score << " / " << cfg::kMaxScore << "\n";
    file << "Cabinet code : " << clues << " (" << (state->cabinetUnlocked ? "unlocked" : "locked") << ")\n";

    file << "\nOBJECTIVE CHECKLIST\n";
    file << "-------------------\n";
    writeObjective(file, state->itemTaken[IT_MASTER_KEY], "Master Key collected", cfg::kScoreMasterKey);
    writeObjective(file, state->clueFour, "Clue digit 4 found", cfg::kScoreClue);
    writeObjective(file, state->clueTwo, "Clue digit 2 found", cfg::kScoreClue);
    writeObjective(file, state->clueSeven, "Clue digit 7 found", cfg::kScoreClue);
    writeObjective(file, state->cabinetUnlocked, "Security cabinet unlocked (427)", cfg::kScoreCabinet);
    writeObjective(file, state->itemTaken[IT_TRAIN_KEY], "Train Key acquired", cfg::kScoreTrainKey);
    writeObjective(file, state->passengerRescued, "Injured passenger rescued", cfg::kScorePassenger);
    writeObjective(file, state->catSecured, "Cat secured in carrier", cfg::kScoreCat);
    writeObjective(file, state->fuseInstalled, "Fuse installed at Control Panel", cfg::kScoreFuse);
    writeObjective(file, state->itemTaken[IT_LIFT_PASS], "Lift Pass collected", cfg::kScoreLiftPass);
    writeObjective(file, state->trainBoarded, "Train prepared and boarded", cfg::kScoreTrain);
    writeObjective(file, state->victory, "Full mission victory bonus", cfg::kScoreVictoryBonus);

    file << "\nAUDIT TRAIL\n";
    file << "-----------\n";
    if (g_auditCount == 0) {
        file << "(no actions recorded)\n";
    }
    for (int i = 0; i < g_auditCount; i++) {
        if (i + 1 < 10) file << "0";
        file << (i + 1) << ". " << g_audit[i] << "\n";
    }
    file << "\nEND OF REPORT\n";

    file.close();
    return true;
}