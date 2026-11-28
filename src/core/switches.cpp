#include "switches.h"
#include <cstring>
#include "physics.h"
#include "util.h"

using namespace std;

void formatClueCode(bool clueFour, bool clueTwo, bool clueSeven, char* out, int cap) {
    if (out == nullptr || cap <= 0) return;
    out[0] = '\0';
    util::appendChar(out, cap, clueFour ? '4' : '_');
    util::appendChar(out, cap, ' ');
    util::appendChar(out, cap, clueTwo ? '2' : '_');
    util::appendChar(out, cap, ' ');
    util::appendChar(out, cap, clueSeven ? '7' : '_');
}

static void setMessage(char* out, int cap, const char* text) {
    util::copyText(out, cap, text);
}

static void appendCostTag(char* out, int cap, int costEnergy, int costTime) {
    util::appendStr(out, cap, "  (cost: ");
    util::appendInt(out, cap, costEnergy);
    util::appendStr(out, cap, " energy, ");
    util::appendInt(out, cap, costTime);
    util::appendStr(out, cap, " time)");
}

static int roomOfItem(const ItemId itemAs[], const int itemRoom[], int itemCount, ItemId id) {
    int index = findItemById(itemAs, itemCount, id);
    return (index >= 0) ? itemRoom[index] : -1;
}

static bool useFuse(int* energy, int* time, int* score, int* room, int* turn,
                    const bool itemTaken[], bool* fuseInstalled, bool* over, bool* victory,
                    const char roomId[][cfg::kIdLen],
                    const ItemId itemAs[], const int itemRoom[], int itemCount,
                    char* out, int cap) {
    if (*fuseInstalled) {
        setMessage(out, cap, "The Fuse is already sitting in the Control Panel.");
        return false;
    }
    if (!hasItem(itemTaken, IT_FUSE)) {
        setMessage(out, cap, "You do not have the Fuse with you.");
        return false;
    }
    int panelRoom = roomOfItem(itemAs, itemRoom, itemCount, IT_PANEL);
    if (*room != panelRoom) {
        setMessage(out, cap, "The Control Panel is in ");
        util::appendStr(out, cap, roomId[panelRoom]);
        util::appendStr(out, cap, ". Walk there before using the Fuse.");
        return false;
    }
    int costEnergy = 0;
    int costTime = 0;
    costUse(&costEnergy, &costTime);
    applyCost(energy, time, costEnergy, costTime);
    (*turn)++;
    *fuseInstalled = true;
    *score += cfg::kScoreFuse;

    setMessage(out, cap, "You fit the Fuse into the Control Panel. +");
    util::appendInt(out, cap, cfg::kScoreFuse);
    util::appendStr(out, cap, " points. Lift 2 still needs the Lift Pass.");
    appendCostTag(out, cap, costEnergy, costTime);
    resolveDefeat(energy, time, over, victory);
    return true;
}

static bool useMedkit(int* energy, int* time, int* score, int* room, int* turn,
                      const bool itemTaken[], bool* passengerRescued, bool* over, bool* victory,
                      const char roomId[][cfg::kIdLen],
                      const ItemId itemAs[], const int itemRoom[], int itemCount,
                      char* out, int cap) {
    if (*passengerRescued) {
        setMessage(out, cap, "The passenger has already been treated and rescued.");
        return false;
    }
    if (!hasItem(itemTaken, IT_MEDKIT)) {
        setMessage(out, cap, "You need the Medical Kit (%) to treat the passenger.");
        return false;
    }
    int passengerRoom = roomOfItem(itemAs, itemRoom, itemCount, IT_PASSENGER);
    if (*room != passengerRoom) {
        setMessage(out, cap, "The injured passenger is in ");
        util::appendStr(out, cap, roomId[passengerRoom]);
        util::appendStr(out, cap, ".");
        return false;
    }
    int costEnergy = 0;
    int costTime = 0;
    costUse(&costEnergy, &costTime);
    applyCost(energy, time, costEnergy, costTime);
    (*turn)++;
    *passengerRescued = true;
    *score += cfg::kScorePassenger;

    setMessage(out, cap, "You treat the passenger's wounds - they can walk now. +");
    util::appendInt(out, cap, cfg::kScorePassenger);
    util::appendStr(out, cap, " points.");
    appendCostTag(out, cap, costEnergy, costTime);
    resolveDefeat(energy, time, over, victory);
    return true;
}

static bool useCatCarrier(int* energy, int* time, int* score, int* room, int* turn,
                          const bool itemTaken[], bool* catSecured, bool* over, bool* victory,
                          const char roomId[][cfg::kIdLen],
                          const ItemId itemAs[], const int itemRoom[], int itemCount,
                          char* out, int cap) {
    if (*catSecured) {
        setMessage(out, cap, "The cat is already safe inside the carrier.");
        return false;
    }
    if (!hasItem(itemTaken, IT_CAT_CARRIER)) {
        setMessage(out, cap, "The cat will not come close without the Cat Carrier (@).");
        return false;
    }
    int catRoom = roomOfItem(itemAs, itemRoom, itemCount, IT_CAT);
    if (*room != catRoom) {
        setMessage(out, cap, "The cat is hiding in ");
        util::appendStr(out, cap, roomId[catRoom]);
        util::appendStr(out, cap, ".");
        return false;
    }
    int costEnergy = 0;
    int costTime = 0;
    costUse(&costEnergy, &costTime);
    applyCost(energy, time, costEnergy, costTime);
    (*turn)++;
    *catSecured = true;
    *score += cfg::kScoreCat;

    setMessage(out, cap, "You coax the cat into the carrier. +");
    util::appendInt(out, cap, cfg::kScoreCat);
    util::appendStr(out, cap, " points.");
    appendCostTag(out, cap, costEnergy, costTime);
    resolveDefeat(energy, time, over, victory);
    return true;
}

static bool unlockCabinet(int* energy, int* time, int* score, int* room, int* turn,
                          bool clueFour, bool clueTwo, bool clueSeven,
                          bool* cabinetUnlocked, bool* over, bool* victory,
                          const char roomId[][cfg::kIdLen],
                          const ItemId itemAs[], const int itemRoom[], int itemCount,
                          int code, char* out, int cap) {
    if (*cabinetUnlocked) {
        setMessage(out, cap, "The security cabinet is already open.");
        return false;
    }
    int cabinetRoom = roomOfItem(itemAs, itemRoom, itemCount, IT_CABINET);
    if (*room != cabinetRoom) {
        setMessage(out, cap, "The security cabinet is in ");
        util::appendStr(out, cap, roomId[cabinetRoom]);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (clueCount(clueFour, clueTwo, clueSeven) < 3) {
        char shown[16];
        formatClueCode(clueFour, clueTwo, clueSeven, shown, sizeof(shown));
        setMessage(out, cap, "The keypad needs three digits. You have ");
        util::appendInt(out, cap, clueCount(clueFour, clueTwo, clueSeven));
        util::appendStr(out, cap, " of 3 clues (");
        util::appendStr(out, cap, shown);
        util::appendStr(out, cap, ").");
        return false;
    }
    if (code != cfg::kCabinetCode) {
        setMessage(out, cap, "The keypad flashes red. Incorrect code.");
        return false;
    }
    int costEnergy = 0;
    int costTime = 0;
    costUse(&costEnergy, &costTime);
    applyCost(energy, time, costEnergy, costTime);
    (*turn)++;
    *cabinetUnlocked = true;
    *score += cfg::kScoreCabinet;

    setMessage(out, cap, "The cabinet clicks open. +");
    util::appendInt(out, cap, cfg::kScoreCabinet);
    util::appendStr(out, cap, " points - a Train Key (^) is inside.");
    appendCostTag(out, cap, costEnergy, costTime);
    resolveDefeat(energy, time, over, victory);
    return true;
}

static bool boardTrain(int* energy, int* time, int* score, int* room, int* turn,
                       const bool itemTaken[], bool clueFour, bool clueTwo, bool clueSeven,
                       bool* fuseInstalled, bool cabinetUnlocked,
                       bool* passengerRescued, bool* catSecured, bool* trainBoarded,
                       bool* over, bool* victory,
                       const char roomId[][cfg::kIdLen], int roomCount,
                       char* out, int cap) {
    if (*trainBoarded) {
        setMessage(out, cap, "You have already boarded the train.");
        return false;
    }
    int platform = findRoom(roomId, roomCount, "F5-R2");
    if (platform < 0) {
        setMessage(out, cap, "Map error: the boarding platform is missing.");
        return false;
    }
    if (*room != platform) {
        setMessage(out, cap, "The train waits at F5-R2 (Train Boarding Platform).");
        return false;
    }
    if (!hasItem(itemTaken, IT_TRAIN_KEY)) {
        setMessage(out, cap, "The train is locked. You need the Train Key.");
        return false;
    }
    if (!*passengerRescued) {
        setMessage(out, cap, "You cannot leave without the injured passenger.");
        return false;
    }
    if (!*catSecured) {
        setMessage(out, cap, "The cat is still loose in F3-R3.");
        return false;
    }
    if (!*fuseInstalled) {
        setMessage(out, cap, "The train has no power. Install the Fuse at the Control Panel.");
        return false;
    }

    int costEnergy = 0;
    int costTime = 0;
    costUse(&costEnergy, &costTime);
    applyCost(energy, time, costEnergy, costTime);
    (*turn)++;
    *trainBoarded = true;
    *score += cfg::kScoreTrain;

    bool full = isFullMission(clueFour, clueTwo, clueSeven, itemTaken, cabinetUnlocked,
                              *passengerRescued, *catSecured, *fuseInstalled, *trainBoarded);
    if (full) {
        *score += cfg::kScoreVictoryBonus;
    }
    *victory = true;
    *over = true;

    setMessage(out, cap, "You board the last train with the passenger and the cat. +");
    util::appendInt(out, cap, cfg::kScoreTrain);
    util::appendStr(out, cap, " points.");
    if (full) {
        util::appendStr(out, cap, " Perfect run - every objective done: +100 bonus.");
    }
    util::appendStr(out, cap, " Final score ");
    util::appendInt(out, cap, *score);
    util::appendStr(out, cap, "/");
    util::appendInt(out, cap, cfg::kMaxScore);
    util::appendStr(out, cap, ".");
    appendCostTag(out, cap, costEnergy, costTime);
    return true;
}

bool switches::tryTraverse(int* energy, int* time, int* room, int* emergencyTrips, int* turn,
                           const bool itemTaken[], bool fuseInstalled,
                           bool* over, bool* victory,
                           const char roomId[][cfg::kIdLen], const char roomName[][cfg::kNameLen],
                           const int roomFloor[], const int linkA[], const int linkB[],
                           const LinkType linkType[], int linkCount, int roomCount,
                           int targetRoom, char* out, int cap) {
    if (targetRoom < 0 || targetRoom >= roomCount) {
        setMessage(out, cap, "That room does not exist.");
        return false;
    }
    int from = *room;
    if (targetRoom == from) {
        setMessage(out, cap, "You are already in ");
        util::appendStr(out, cap, roomId[from]);
        util::appendStr(out, cap, ".");
        return false;
    }

    LinkType type = findLink(linkA, linkB, linkType, linkCount, from, targetRoom);
    if (type == LINK_NONE) {
        char exits[512];
        buildExitList(roomId, linkA, linkB, linkType, linkCount, from, exits, (int)sizeof(exits));
        setMessage(out, cap, "No direct passage from ");
        util::appendStr(out, cap, roomId[from]);
        util::appendStr(out, cap, " to ");
        util::appendStr(out, cap, roomId[targetRoom]);
        util::appendStr(out, cap, ". ");
        util::appendStr(out, cap, exits);
        return false;
    }
    if (type == LINK_WALL) {
        setMessage(out, cap, "A solid wall separates ");
        util::appendStr(out, cap, roomId[from]);
        util::appendStr(out, cap, " and ");
        util::appendStr(out, cap, roomId[targetRoom]);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (type == LINK_LOCKED && !hasItem(itemTaken, IT_MASTER_KEY)) {
        setMessage(out, cap, "The door between ");
        util::appendStr(out, cap, roomId[from]);
        util::appendStr(out, cap, " and ");
        util::appendStr(out, cap, roomId[targetRoom]);
        util::appendStr(out, cap, " is locked. The Master Key is required.");
        return false;
    }

    bool emergency = false;
    if (type == LINK_LIFT2 && !(fuseInstalled && hasItem(itemTaken, IT_LIFT_PASS))) {
        if (!hasItem(itemTaken, IT_ACCESS_CARD)) {
            setMessage(out, cap,
                       "Lift 2 is unpowered. Install the Fuse and hold the Lift Pass, or find the Emergency Access Card.");
            return false;
        }
        if (*emergencyTrips <= 0) {
            setMessage(out, cap, "The Emergency Access Card is used up. Repair Lift 2 to ride it.");
            return false;
        }
        emergency = true;
    }

    int costEnergy = 0;
    int costTime = 0;
    costForMove(type, &costEnergy, &costTime);
    if (emergency) {
        (*emergencyTrips)--;
    }
    applyCost(energy, time, costEnergy, costTime);
    *room = targetRoom;
    (*turn)++;

    switch (type) {
        case LINK_DOOR:
            setMessage(out, cap, "You step through the door into ");
            break;
        case LINK_WINDOW:
            setMessage(out, cap, "You climb through the window into ");
            break;
        case LINK_STAIR:
            setMessage(out, cap, roomFloor[targetRoom] > roomFloor[from]
                                     ? "You take the stairs up to "
                                     : "You take the stairs down to ");
            break;
        case LINK_LOCKED:
            setMessage(out, cap, "You unlock the door with the Master Key and step into ");
            break;
        case LINK_LIFT1:
            setMessage(out, cap, "You ride lift 1 to ");
            break;
        case LINK_LIFT2:
            setMessage(out, cap, emergency ? "You ride lift 2 on an emergency trip to "
                                           : "You ride lift 2 to ");
            break;
        default:
            setMessage(out, cap, "You make your way to ");
            break;
    }
    util::appendStr(out, cap, roomName[targetRoom]);
    util::appendStr(out, cap, " (");
    util::appendStr(out, cap, roomId[targetRoom]);
    util::appendStr(out, cap, ").");
    if (emergency) {
        util::appendStr(out, cap, " Emergency trips left: ");
        util::appendInt(out, cap, *emergencyTrips);
        util::appendStr(out, cap, ".");
    }
    appendCostTag(out, cap, costEnergy, costTime);
    resolveDefeat(energy, time, over, victory);
    return true;
}

bool switches::tryCollect(int* energy, int* time, int* score, int* room, int* turn,
                          int emergencyTrips,
                          bool itemTaken[], bool* clueFour, bool* clueTwo, bool* clueSeven,
                          bool cabinetUnlocked, bool* over, bool* victory,
                          const char roomId[][cfg::kIdLen],
                          const char itemCode[][cfg::kItemIdLen], const char itemName[][cfg::kNameLen],
                          const char itemSymbol[], const int itemRoom[], const ItemClass itemClass[],
                          const ItemId itemAs[], const int itemValue[], int itemCount,
                          const char* token, char* out, int cap) {
    int index = findItemDef(itemCode, itemName, itemSymbol, itemCount, token);
    if (index < 0) {
        setMessage(out, cap, "There is nothing called '");
        util::appendStr(out, cap, token);
        util::appendStr(out, cap, "' here.");
        return false;
    }
    ItemId id = itemAs[index];

    if (itemTaken[id]) {
        setMessage(out, cap, "You already have the ");
        util::appendStr(out, cap, itemName[index]);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (itemRoom[index] != *room) {
        setMessage(out, cap, "There is no ");
        util::appendStr(out, cap, itemName[index]);
        util::appendStr(out, cap, " in ");
        util::appendStr(out, cap, roomId[*room]);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (itemClass[index] == CLASS_ENTITY) {
        if (id == IT_CAT) {
            setMessage(out, cap, "The cat keeps its distance. You need the Cat Carrier (@).");
        } else {
            setMessage(out, cap, "The passenger is too injured to walk. Use the Medical Kit (%) here.");
        }
        return false;
    }
    if (itemClass[index] == CLASS_FIXED) {
        if (id == IT_CABINET) {
            setMessage(out, cap, "The cabinet is locked. Try: UNLOCK CABINET 427.");
        } else {
            setMessage(out, cap, "The panel is bolted to the wall. Try: USE FUSE here.");
        }
        return false;
    }
    if (id == IT_TRAIN_KEY && !cabinetUnlocked) {
        setMessage(out, cap, "The security cabinet is still closed.");
        return false;
    }

    int costEnergy = 0;
    int costTime = 0;
    costPickup(&costEnergy, &costTime);
    applyCost(energy, time, costEnergy, costTime);
    (*turn)++;
    itemTaken[id] = true;

    switch (id) {
        case IT_ENERGY_PACK_A:
        case IT_ENERGY_PACK_B:
            addEnergy(energy, itemValue[index]);
            clampResources(energy, time);
            setMessage(out, cap, "You pocket an Energy Pack. +");
            util::appendInt(out, cap, itemValue[index]);
            util::appendStr(out, cap, " energy (");
            util::appendInt(out, cap, *energy);
            util::appendStr(out, cap, "/100).");
            break;
        case IT_TOOLKIT:
            addEnergy(energy, itemValue[index]);
            clampResources(energy, time);
            setMessage(out, cap, "You grab the Emergency Toolkit. +");
            util::appendInt(out, cap, itemValue[index]);
            util::appendStr(out, cap, " energy (");
            util::appendInt(out, cap, *energy);
            util::appendStr(out, cap, "/100).");
            break;
        case IT_OBSERVATION_TOKEN:
            addTime(time, itemValue[index]);
            clampResources(energy, time);
            setMessage(out, cap, "You take the Observation Token. +");
            util::appendInt(out, cap, itemValue[index]);
            util::appendStr(out, cap, " time (");
            util::appendInt(out, cap, *time);
            util::appendStr(out, cap, "/100).");
            break;
        case IT_MASTER_KEY:
            *score += cfg::kScoreMasterKey;
            setMessage(out, cap, "You take the Master Key. +");
            util::appendInt(out, cap, cfg::kScoreMasterKey);
            util::appendStr(out, cap, " points - it opens the locked door on floor 3.");
            break;
        case IT_CLUE_FOUR:
        case IT_CLUE_TWO:
        case IT_CLUE_SEVEN: {
            if (id == IT_CLUE_FOUR) *clueFour = true;
            else if (id == IT_CLUE_TWO) *clueTwo = true;
            else *clueSeven = true;

            *score += cfg::kScoreClue;
            char code[16];
            formatClueCode(*clueFour, *clueTwo, *clueSeven, code, sizeof(code));
            setMessage(out, cap, "You read clue digit ");
            util::appendInt(out, cap, itemValue[index]);
            util::appendStr(out, cap, ". +");
            util::appendInt(out, cap, cfg::kScoreClue);
            util::appendStr(out, cap, " points. Code so far: ");
            util::appendStr(out, cap, code);
            break;
        }
        case IT_LIFT_PASS:
            *score += cfg::kScoreLiftPass;
            setMessage(out, cap, "You take the Lift Pass. +");
            util::appendInt(out, cap, cfg::kScoreLiftPass);
            util::appendStr(out, cap, " points - lift 2 runs normally once the Fuse is installed.");
            break;
        case IT_TRAIN_KEY:
            *score += cfg::kScoreTrainKey;
            setMessage(out, cap, "You take the Train Key. +");
            util::appendInt(out, cap, cfg::kScoreTrainKey);
            util::appendStr(out, cap, " points.");
            break;
        case IT_ACCESS_CARD:
            setMessage(out, cap, "Emergency Access Card collected: up to ");
            util::appendInt(out, cap, emergencyTrips);
            util::appendStr(out, cap, " Lift 2 trips before repair.");
            break;
        default:
            setMessage(out, cap, itemName[index]);
            util::appendStr(out, cap, " collected.");
            break;
    }

    appendCostTag(out, cap, costEnergy, costTime);
    resolveDefeat(energy, time, over, victory);
    return true;
}

bool switches::tryUse(int* energy, int* time, int* score, int* room, int* turn,
                      bool itemTaken[], bool* clueFour, bool* clueTwo, bool* clueSeven,
                      bool* fuseInstalled, bool* cabinetUnlocked, bool* passengerRescued,
                      bool* catSecured, bool* trainBoarded, bool* over, bool* victory,
                      const char roomId[][cfg::kIdLen], int roomCount,
                      const char itemName[][cfg::kNameLen], const int itemRoom[],
                      const ItemId itemAs[], int itemCount,
                      ItemId target, int argument, char* out, int cap) {
    switch (target) {
        case IT_FUSE:
            return useFuse(energy, time, score, room, turn, itemTaken, fuseInstalled,
                           over, victory, roomId, itemAs, itemRoom, itemCount, out, cap);
        case IT_MEDKIT:
            return useMedkit(energy, time, score, room, turn, itemTaken, passengerRescued,
                             over, victory, roomId, itemAs, itemRoom, itemCount, out, cap);
        case IT_CAT_CARRIER:
            return useCatCarrier(energy, time, score, room, turn, itemTaken, catSecured,
                                 over, victory, roomId, itemAs, itemRoom, itemCount, out, cap);
        case IT_CABINET:
            return unlockCabinet(energy, time, score, room, turn,
                                 *clueFour, *clueTwo, *clueSeven, cabinetUnlocked,
                                 over, victory, roomId, itemAs, itemRoom, itemCount,
                                 argument, out, cap);
        case IT_TRAIN_KEY:
            return boardTrain(energy, time, score, room, turn, itemTaken,
                              *clueFour, *clueTwo, *clueSeven, fuseInstalled, *cabinetUnlocked,
                              passengerRescued, catSecured, trainBoarded, over, victory,
                              roomId, roomCount, out, cap);
        default: {
            int index = findItemById(itemAs, itemCount, target);
            setMessage(out, cap, "You cannot use the ");
            util::appendStr(out, cap, index >= 0 ? itemName[index] : "item");
            util::appendStr(out, cap, " right now.");
            return false;
        }
    }
}