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

bool logger::saveGameStateToFile(int energy, int time, int score, int room, int turn,
                                 int emergencyTrips,
                                 bool fuseInstalled, bool cabinetUnlocked, bool passengerRescued,
                                 bool catSecured, bool trainBoarded,
                                 const bool itemTaken[],
                                 const char roomId[][cfg::kIdLen],
                                 const char itemCode[][cfg::kItemIdLen],
                                 const ItemId itemAs[], int itemCount,
                                 const char* path, char* error, int errorCap) {
    ofstream file(path);
    if (!file.is_open()) {
        util::copyText(error, errorCap, "Cannot open file for writing: ");
        util::appendStr(error, errorCap, path);
        return false;
    }

    file << "VERSION 1\n";
    file << "TURN " << turn << "\n";
    file << "ROOM " << roomId[room] << "\n";
    file << "ENERGY " << energy << "\n";
    file << "TIME " << time << "\n";
    file << "SCORE " << score << "\n";
    file << "TRIPS " << emergencyTrips << "\n";
    file << "FUSE_INSTALLED " << (fuseInstalled ? 1 : 0) << "\n";
    file << "CABINET_OPEN " << (cabinetUnlocked ? 1 : 0) << "\n";
    file << "PASSENGER_RESCUED " << (passengerRescued ? 1 : 0) << "\n";
    file << "CAT_SECURED " << (catSecured ? 1 : 0) << "\n";
    file << "TRAIN_BOARDED " << (trainBoarded ? 1 : 0) << "\n";

    file << "TAKEN";
    bool any = false;
    for (int i = 0; i < itemCount; i++) {
        if (itemTaken[itemAs[i]]) {
            file << (any ? "," : " ") << itemCode[i];
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

bool logger::loadGameStateFromFile(int* energy, int* time, int* score, int* room, int* turn,
                                   int* emergencyTrips,
                                   bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
                                   bool* catSecured, bool* trainBoarded,
                                   bool* clueFour, bool* clueTwo, bool* clueSeven,
                                   bool itemTaken[], bool* over, bool* victory,
                                   const char roomId[][cfg::kIdLen], int roomCount,
                                   const char itemCode[][cfg::kItemIdLen],
                                   const char itemName[][cfg::kNameLen],
                                   const char itemSymbol[], const ItemId itemAs[], int itemCount,
                                   const char* path, char* error, int errorCap) {
    ifstream file(path);
    if (!file.is_open()) {
        util::copyText(error, errorCap, "Cannot open file for reading: ");
        util::appendStr(error, errorCap, path);
        return false;
    }

    int tEnergy = 0;
    int tTime = 0;
    int tScore = 0;
    int tRoom = -1;
    int tTurn = 0;
    int tTrips = cfg::kStartEmergencyTrips;
    bool tFuse = false;
    bool tCabinet = false;
    bool tPassenger = false;
    bool tCat = false;
    bool tBoarded = false;
    bool tItemTaken[IT_COUNT] = {false};
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
            ok = parseRange(val, 0, 100000, &tTurn);
            seenTurn = ok;
        } else if (strcmp(key, "ROOM") == 0) {
            int found = findRoom(roomId, roomCount, val);
            if (found < 0) {
                ok = false;
                util::copyText(reason, sizeof(reason), "unknown ROOM id");
            } else {
                tRoom = found;
                seenRoom = true;
            }
        } else if (strcmp(key, "ENERGY") == 0) {
            ok = parseRange(val, 0, cfg::kResourceCap, &tEnergy);
            seenEnergy = ok;
        } else if (strcmp(key, "TIME") == 0) {
            ok = parseRange(val, 0, cfg::kResourceCap, &tTime);
            seenTime = ok;
        } else if (strcmp(key, "SCORE") == 0) {
            ok = parseRange(val, 0, cfg::kMaxScore, &tScore);
            seenScore = ok;
        } else if (strcmp(key, "TRIPS") == 0) {
            ok = parseRange(val, 0, cfg::kStartEmergencyTrips, &tTrips);
            seenTrips = ok;
        } else if (strcmp(key, "FUSE_INSTALLED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            tFuse = (flag != 0);
            seenFuse = ok;
        } else if (strcmp(key, "CABINET_OPEN") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            tCabinet = (flag != 0);
            seenCabinet = ok;
        } else if (strcmp(key, "PASSENGER_RESCUED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            tPassenger = (flag != 0);
            seenPassenger = ok;
        } else if (strcmp(key, "CAT_SECURED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            tCat = (flag != 0);
            seenCat = ok;
        } else if (strcmp(key, "TRAIN_BOARDED") == 0) {
            int flag = 0;
            ok = parseRange(val, 0, 1, &flag);
            tBoarded = (flag != 0);
            seenBoarded = ok;
        } else if (strcmp(key, "TAKEN") == 0) {
            seenTaken = true;
            char* cursor = val;
            while (ok && *cursor != '\0') {
                char* comma = strchr(cursor, ',');
                if (comma != nullptr) *comma = '\0';
                util::trimInPlace(cursor);

                if (*cursor != '\0' && strcmp(cursor, "-") != 0) {
                    int index = findItemDef(itemCode, itemName, itemSymbol, itemCount, cursor);
                    if (index < 0) {
                        ok = false;
                        util::copyText(reason, sizeof(reason), "unknown item in TAKEN");
                    } else {
                        tItemTaken[itemAs[index]] = true;
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

    bool tClueFour = tItemTaken[IT_CLUE_FOUR];
    bool tClueTwo = tItemTaken[IT_CLUE_TWO];
    bool tClueSeven = tItemTaken[IT_CLUE_SEVEN];
    bool tOver = false;
    bool tVictory = false;
    if (tBoarded) {
        tOver = true;
        tVictory = true;
    } else if (tEnergy <= 0 || tTime <= 0) {
        tOver = true;
        tVictory = false;
    }

    *energy = tEnergy;
    *time = tTime;
    *score = tScore;
    *room = tRoom;
    *turn = tTurn;
    *emergencyTrips = tTrips;
    *fuseInstalled = tFuse;
    *cabinetUnlocked = tCabinet;
    *passengerRescued = tPassenger;
    *catSecured = tCat;
    *trainBoarded = tBoarded;
    *clueFour = tClueFour;
    *clueTwo = tClueTwo;
    *clueSeven = tClueSeven;
    for (int i = 0; i < IT_COUNT; i++) {
        itemTaken[i] = tItemTaken[i];
    }
    *over = tOver;
    *victory = tVictory;
    return true;
}

bool logger::writeGameSummaryReport(int energy, int time, int score, int room, int turn,
                                    bool fuseInstalled, bool cabinetUnlocked, bool passengerRescued,
                                    bool catSecured, bool trainBoarded, bool victory,
                                    bool clueFour, bool clueTwo, bool clueSeven,
                                    const bool itemTaken[],
                                    const char roomId[][cfg::kIdLen],
                                    const char roomName[][cfg::kNameLen],
                                    const char* result, const char* path,
                                    char* error, int errorCap) {
    ofstream file(path);
    if (!file.is_open()) {
        util::copyText(error, errorCap, "Cannot open file for writing: ");
        util::appendStr(error, errorCap, path);
        return false;
    }

    char clues[16];
    formatClueCode(clueFour, clueTwo, clueSeven, clues, (int)sizeof(clues));

    file << "THE LAST TRAIN: SESSION SUMMARY\n";
    file << "Result       : " << result << "\n";
    file << "Turn count   : " << turn << "\n";
    file << "Final room   : " << roomId[room] << " (" << roomName[room] << ")\n";
    file << "Energy       : " << energy << " / " << cfg::kResourceCap << "\n";
    file << "Time         : " << time << " / " << cfg::kResourceCap << "\n";
    file << "Final score  : " << score << " / " << cfg::kMaxScore << "\n";
    file << "Cabinet code : " << clues << " (" << (cabinetUnlocked ? "unlocked" : "locked") << ")\n";

    file << "\nOBJECTIVE CHECKLIST\n\n";
    writeObjective(file, itemTaken[IT_MASTER_KEY], "Master Key collected", cfg::kScoreMasterKey);
    writeObjective(file, clueFour, "Clue digit 4 found", cfg::kScoreClue);
    writeObjective(file, clueTwo, "Clue digit 2 found", cfg::kScoreClue);
    writeObjective(file, clueSeven, "Clue digit 7 found", cfg::kScoreClue);
    writeObjective(file, cabinetUnlocked, "Security cabinet unlocked (427)", cfg::kScoreCabinet);
    writeObjective(file, itemTaken[IT_TRAIN_KEY], "Train Key acquired", cfg::kScoreTrainKey);
    writeObjective(file, passengerRescued, "Injured passenger rescued", cfg::kScorePassenger);
    writeObjective(file, catSecured, "Cat secured in carrier", cfg::kScoreCat);
    writeObjective(file, fuseInstalled, "Fuse installed at Control Panel", cfg::kScoreFuse);
    writeObjective(file, itemTaken[IT_LIFT_PASS], "Lift Pass collected", cfg::kScoreLiftPass);
    writeObjective(file, trainBoarded, "Train prepared and boarded", cfg::kScoreTrain);
    writeObjective(file, victory, "Full mission victory bonus", cfg::kScoreVictoryBonus);

    file << "\nAUDIT TRAIL\n\n";
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