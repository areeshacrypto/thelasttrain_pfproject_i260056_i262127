#include "switches.h"
#include <cstring>
#include "physics.h"
#include "util.h"

using namespace std;

void formatClueCode(const GameState* state, char* out, int cap) {
    if (out == nullptr || cap <= 0) return;
    out[0] = '\0';
    util::appendChar(out, cap, state->clueFour ? '4' : '_');
    util::appendChar(out, cap, ' ');
    util::appendChar(out, cap, state->clueTwo ? '2' : '_');
    util::appendChar(out, cap, ' ');
    util::appendChar(out, cap, state->clueSeven ? '7' : '_');
}

static void setMessage(char* out, int cap, const char* text) {
    util::copyText(out, cap, text);
}

static void appendCostTag(char* out, int cap, Cost cost) {
    util::appendStr(out, cap, "  (cost: ");
    util::appendInt(out, cap, cost.energy);
    util::appendStr(out, cap, " energy, ");
    util::appendInt(out, cap, cost.time);
    util::appendStr(out, cap, " time)");
}

static int roomOfItem(const BuildingMap* map, ItemId id) {
    const ItemDef* def = findItemById(map, id);
    return (def != nullptr) ? def->room : -1;
}

static bool useFuse(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->fuseInstalled) {
        setMessage(out, cap, "The Fuse is already sitting in the Control Panel.");
        return false;
    }
    if (!hasItem(state, IT_FUSE)) {
        setMessage(out, cap, "You do not have the Fuse with you.");
        return false;
    }
    int panelRoom = roomOfItem(map, IT_PANEL);
    if (state->room != panelRoom) {
        setMessage(out, cap, "The Control Panel is in ");
        util::appendStr(out, cap, map->rooms[panelRoom].id);
        util::appendStr(out, cap, ". Walk there before using the Fuse.");
        return false;
    }
    Cost cost = costUse();
    applyCost(state, cost);
    state->turn++;
    state->fuseInstalled = true;
    state->score += cfg::kScoreFuse;

    setMessage(out, cap, "You fit the Fuse into the Control Panel. +");
    util::appendInt(out, cap, cfg::kScoreFuse);
    util::appendStr(out, cap, " points. Lift 2 still needs the Lift Pass.");
    appendCostTag(out, cap, cost);
    resolveDefeat(state);
    return true;
}

static bool useMedkit(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->passengerRescued) {
        setMessage(out, cap, "The passenger has already been treated and rescued.");
        return false;
    }
    if (!hasItem(state, IT_MEDKIT)) {
        setMessage(out, cap, "You need the Medical Kit (%) to treat the passenger.");
        return false;
    }
    int passengerRoom = roomOfItem(map, IT_PASSENGER);
    if (state->room != passengerRoom) {
        setMessage(out, cap, "The injured passenger is in ");
        util::appendStr(out, cap, map->rooms[passengerRoom].id);
        util::appendStr(out, cap, ".");
        return false;
    }
    Cost cost = costUse();
    applyCost(state, cost);
    state->turn++;
    state->passengerRescued = true;
    state->score += cfg::kScorePassenger;

    setMessage(out, cap, "You treat the passenger's wounds - they can walk now. +");
    util::appendInt(out, cap, cfg::kScorePassenger);
    util::appendStr(out, cap, " points.");
    appendCostTag(out, cap, cost);
    resolveDefeat(state);
    return true;
}

static bool useCatCarrier(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->catSecured) {
        setMessage(out, cap, "The cat is already safe inside the carrier.");
        return false;
    }
    if (!hasItem(state, IT_CAT_CARRIER)) {
        setMessage(out, cap, "The cat will not come close without the Cat Carrier (@).");
        return false;
    }
    int catRoom = roomOfItem(map, IT_CAT);
    if (state->room != catRoom) {
        setMessage(out, cap, "The cat is hiding in ");
        util::appendStr(out, cap, map->rooms[catRoom].id);
        util::appendStr(out, cap, ".");
        return false;
    }
    Cost cost = costUse();
    applyCost(state, cost);
    state->turn++;
    state->catSecured = true;
    state->score += cfg::kScoreCat;

    setMessage(out, cap, "You coax the cat into the carrier. +");
    util::appendInt(out, cap, cfg::kScoreCat);
    util::appendStr(out, cap, " points.");
    appendCostTag(out, cap, cost);
    resolveDefeat(state);
    return true;
}

static bool unlockCabinet(GameState* state, const BuildingMap* map, int code, char* out, int cap) {
    if (state->cabinetUnlocked) {
        setMessage(out, cap, "The security cabinet is already open.");
        return false;
    }
    int cabinetRoom = roomOfItem(map, IT_CABINET);
    if (state->room != cabinetRoom) {
        setMessage(out, cap, "The security cabinet is in ");
        util::appendStr(out, cap, map->rooms[cabinetRoom].id);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (clueCount(state) < 3) {
        char shown[16];
        formatClueCode(state, shown, sizeof(shown));
        setMessage(out, cap, "The keypad needs three digits. You have ");
        util::appendInt(out, cap, clueCount(state));
        util::appendStr(out, cap, " of 3 clues (");
        util::appendStr(out, cap, shown);
        util::appendStr(out, cap, ").");
        return false;
    }
    if (code != cfg::kCabinetCode) {
        setMessage(out, cap, "The keypad flashes red. Incorrect code.");
        return false;
    }
    Cost cost = costUse();
    applyCost(state, cost);
    state->turn++;
    state->cabinetUnlocked = true;
    state->score += cfg::kScoreCabinet;

    setMessage(out, cap, "The cabinet clicks open. +");
    util::appendInt(out, cap, cfg::kScoreCabinet);
    util::appendStr(out, cap, " points - a Train Key (^) is inside.");
    appendCostTag(out, cap, cost);
    resolveDefeat(state);
    return true;
}

static bool boardTrain(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->trainBoarded) {
        setMessage(out, cap, "You have already boarded the train.");
        return false;
    }
    int platform = findRoom(map, "F5-R2");
    if (platform < 0) {
        setMessage(out, cap, "Map error: the boarding platform is missing.");
        return false;
    }
    if (state->room != platform) {
        setMessage(out, cap, "The train waits at F5-R2 (Train Boarding Platform).");
        return false;
    }
    if (!hasItem(state, IT_TRAIN_KEY)) {
        setMessage(out, cap, "The train is locked. You need the Train Key.");
        return false;
    }
    if (!state->passengerRescued) {
        setMessage(out, cap, "You cannot leave without the injured passenger.");
        return false;
    }
    if (!state->catSecured) {
        setMessage(out, cap, "The cat is still loose in F3-R3.");
        return false;
    }
    if (!state->fuseInstalled) {
        setMessage(out, cap, "The train has no power. Install the Fuse at the Control Panel.");
        return false;
    }

    Cost cost = costUse();
    applyCost(state, cost);
    state->turn++;
    state->trainBoarded = true;
    state->score += cfg::kScoreTrain;

    bool full = isFullMission(state);
    if (full) {
        state->score += cfg::kScoreVictoryBonus;
    }
    state->victory = true;
    state->over = true;

    setMessage(out, cap, "You board the last train with the passenger and the cat. +");
    util::appendInt(out, cap, cfg::kScoreTrain);
    util::appendStr(out, cap, " points.");
    if (full) {
        util::appendStr(out, cap, " Perfect run - every objective done: +100 bonus.");
    }
    util::appendStr(out, cap, " Final score ");
    util::appendInt(out, cap, state->score);
    util::appendStr(out, cap, "/");
    util::appendInt(out, cap, cfg::kMaxScore);
    util::appendStr(out, cap, ".");
    appendCostTag(out, cap, cost);
    return true;
}

bool switches::tryTraverse(GameState* state, const BuildingMap* map, int targetRoom, char* out, int cap) {
    if (targetRoom < 0 || targetRoom >= map->roomCount) {
        setMessage(out, cap, "That room does not exist.");
        return false;
    }
    int from = state->room;
    if (targetRoom == from) {
        setMessage(out, cap, "You are already in ");
        util::appendStr(out, cap, map->rooms[from].id);
        util::appendStr(out, cap, ".");
        return false;
    }

    LinkType type = findLink(map, from, targetRoom);
    if (type == LINK_NONE) {
        char exits[512];
        buildExitList(map, from, exits, (int)sizeof(exits));
        setMessage(out, cap, "No direct passage from ");
        util::appendStr(out, cap, map->rooms[from].id);
        util::appendStr(out, cap, " to ");
        util::appendStr(out, cap, map->rooms[targetRoom].id);
        util::appendStr(out, cap, ". ");
        util::appendStr(out, cap, exits);
        return false;
    }
    if (type == LINK_WALL) {
        setMessage(out, cap, "A solid wall separates ");
        util::appendStr(out, cap, map->rooms[from].id);
        util::appendStr(out, cap, " and ");
        util::appendStr(out, cap, map->rooms[targetRoom].id);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (type == LINK_LOCKED && !hasItem(state, IT_MASTER_KEY)) {
        setMessage(out, cap, "The door between ");
        util::appendStr(out, cap, map->rooms[from].id);
        util::appendStr(out, cap, " and ");
        util::appendStr(out, cap, map->rooms[targetRoom].id);
        util::appendStr(out, cap, " is locked. The Master Key is required.");
        return false;
    }

    bool emergency = false;
    if (type == LINK_LIFT2 && !(state->fuseInstalled && hasItem(state, IT_LIFT_PASS))) {
        if (!hasItem(state, IT_ACCESS_CARD)) {
            setMessage(out, cap,
                       "Lift 2 is unpowered. Install the Fuse and hold the Lift Pass, or find the Emergency Access Card.");
            return false;
        }
        if (state->emergencyTrips <= 0) {
            setMessage(out, cap, "The Emergency Access Card is used up. Repair Lift 2 to ride it.");
            return false;
        }
        emergency = true;
    }

    Cost cost = costForMove(type);
    if (emergency) {
        state->emergencyTrips--;
    }
    applyCost(state, cost);
    state->room = targetRoom;
    state->turn++;

    const Room& room = map->rooms[targetRoom];
    switch (type) {
        case LINK_DOOR:
            setMessage(out, cap, "You step through the door into ");
            break;
        case LINK_WINDOW:
            setMessage(out, cap, "You climb through the window into ");
            break;
        case LINK_STAIR:
            setMessage(out, cap, room.floorIdx > map->rooms[from].floorIdx
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
    util::appendStr(out, cap, room.name);
    util::appendStr(out, cap, " (");
    util::appendStr(out, cap, room.id);
    util::appendStr(out, cap, ").");
    if (emergency) {
        util::appendStr(out, cap, " Emergency trips left: ");
        util::appendInt(out, cap, state->emergencyTrips);
        util::appendStr(out, cap, ".");
    }
    appendCostTag(out, cap, cost);
    resolveDefeat(state);
    return true;
}

bool switches::tryCollect(GameState* state, const BuildingMap* map, const char* token, char* out, int cap) {
    int index = findItemDef(map, token);
    if (index < 0) {
        setMessage(out, cap, "There is nothing called '");
        util::appendStr(out, cap, token);
        util::appendStr(out, cap, "' here.");
        return false;
    }
    const ItemDef& def = map->items[index];

    if (state->itemTaken[def.itemId]) {
        setMessage(out, cap, "You already have the ");
        util::appendStr(out, cap, def.name);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (def.room != state->room) {
        setMessage(out, cap, "There is no ");
        util::appendStr(out, cap, def.name);
        util::appendStr(out, cap, " in ");
        util::appendStr(out, cap, map->rooms[state->room].id);
        util::appendStr(out, cap, ".");
        return false;
    }
    if (def.cls == CLASS_ENTITY) {
        if (def.itemId == IT_CAT) {
            setMessage(out, cap, "The cat keeps its distance. You need the Cat Carrier (@).");
        } else {
            setMessage(out, cap, "The passenger is too injured to walk. Use the Medical Kit (%) here.");
        }
        return false;
    }
    if (def.cls == CLASS_FIXED) {
        if (def.itemId == IT_CABINET) {
            setMessage(out, cap, "The cabinet is locked. Try: UNLOCK CABINET 427.");
        } else {
            setMessage(out, cap, "The panel is bolted to the wall. Try: USE FUSE here.");
        }
        return false;
    }
    if (def.itemId == IT_TRAIN_KEY && !state->cabinetUnlocked) {
        setMessage(out, cap, "The security cabinet is still closed.");
        return false;
    }

    Cost cost = costPickup();
    applyCost(state, cost);
    state->turn++;
    state->itemTaken[def.itemId] = true;

    switch (def.itemId) {
        case IT_ENERGY_PACK_A:
        case IT_ENERGY_PACK_B:
            addEnergy(state, def.value);
            clampResources(state);
            setMessage(out, cap, "You pocket an Energy Pack. +");
            util::appendInt(out, cap, def.value);
            util::appendStr(out, cap, " energy (");
            util::appendInt(out, cap, state->energy);
            util::appendStr(out, cap, "/100).");
            break;
        case IT_TOOLKIT:
            addEnergy(state, def.value);
            clampResources(state);
            setMessage(out, cap, "You grab the Emergency Toolkit. +");
            util::appendInt(out, cap, def.value);
            util::appendStr(out, cap, " energy (");
            util::appendInt(out, cap, state->energy);
            util::appendStr(out, cap, "/100).");
            break;
        case IT_OBSERVATION_TOKEN:
            addTime(state, def.value);
            clampResources(state);
            setMessage(out, cap, "You take the Observation Token. +");
            util::appendInt(out, cap, def.value);
            util::appendStr(out, cap, " time (");
            util::appendInt(out, cap, state->time);
            util::appendStr(out, cap, "/100).");
            break;
        case IT_MASTER_KEY:
            state->score += cfg::kScoreMasterKey;
            setMessage(out, cap, "You take the Master Key. +");
            util::appendInt(out, cap, cfg::kScoreMasterKey);
            util::appendStr(out, cap, " points - it opens the locked door on floor 3.");
            break;
        case IT_CLUE_FOUR:
        case IT_CLUE_TWO:
        case IT_CLUE_SEVEN: {
            if (def.itemId == IT_CLUE_FOUR) state->clueFour = true;
            else if (def.itemId == IT_CLUE_TWO) state->clueTwo = true;
            else state->clueSeven = true;

            state->score += cfg::kScoreClue;
            char code[16];
            formatClueCode(state, code, sizeof(code));
            setMessage(out, cap, "You read clue digit ");
            util::appendInt(out, cap, def.value);
            util::appendStr(out, cap, ". +");
            util::appendInt(out, cap, cfg::kScoreClue);
            util::appendStr(out, cap, " points. Code so far: ");
            util::appendStr(out, cap, code);
            break;
        }
        case IT_LIFT_PASS:
            state->score += cfg::kScoreLiftPass;
            setMessage(out, cap, "You take the Lift Pass. +");
            util::appendInt(out, cap, cfg::kScoreLiftPass);
            util::appendStr(out, cap, " points - lift 2 runs normally once the Fuse is installed.");
            break;
        case IT_TRAIN_KEY:
            state->score += cfg::kScoreTrainKey;
            setMessage(out, cap, "You take the Train Key. +");
            util::appendInt(out, cap, cfg::kScoreTrainKey);
            util::appendStr(out, cap, " points.");
            break;
        case IT_ACCESS_CARD:
            setMessage(out, cap, "Emergency Access Card collected: up to ");
            util::appendInt(out, cap, state->emergencyTrips);
            util::appendStr(out, cap, " Lift 2 trips before repair.");
            break;
        default:
            setMessage(out, cap, def.name);
            util::appendStr(out, cap, " collected.");
            break;
    }

    appendCostTag(out, cap, cost);
    resolveDefeat(state);
    return true;
}

bool switches::tryUse(GameState* state, const BuildingMap* map, ItemId target, int argument, char* out, int cap) {
    switch (target) {
        case IT_FUSE:        return useFuse(state, map, out, cap);
        case IT_MEDKIT:      return useMedkit(state, map, out, cap);
        case IT_CAT_CARRIER: return useCatCarrier(state, map, out, cap);
        case IT_CABINET:     return unlockCabinet(state, map, argument, out, cap);
        case IT_TRAIN_KEY:   return boardTrain(state, map, out, cap);
        default: {
            const ItemDef* def = findItemById(map, target);
            setMessage(out, cap, "You cannot use the ");
            util::appendStr(out, cap, def != nullptr ? def->name : "item");
            util::appendStr(out, cap, " right now.");
            return false;
        }
    }
}