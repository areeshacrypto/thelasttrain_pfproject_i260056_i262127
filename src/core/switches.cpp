#include "switches.h"

#include <cstdio>
#include <cstring>

#include "physics.h"
#include "util.h"

void formatClueCode(const GameState* state, char* out, int cap) {
    std::snprintf(out, static_cast<size_t>(cap), "%c %c %c",
                  state->clueFour ? '4' : '_',
                  state->clueTwo ? '2' : '_',
                  state->clueSeven ? '7' : '_');
}

namespace {

void setMessage(char* out, int cap, const char* text) {
    util::copyText(out, cap, text);
}

int roomOfItem(const BuildingMap* map, ItemId id) {
    const ItemDef* def = findItemById(map, id);
    return def != nullptr ? def->room : -1;
}

bool useFuse(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->fuseInstalled) {
        setMessage(out, cap, "The Fuse is already installed in the Control Panel.");
        return false;
    }
    if (!hasItem(state, IT_FUSE)) {
        setMessage(out, cap, "You do not have the Fuse.");
        return false;
    }
    const int panelRoom = roomOfItem(map, IT_PANEL);
    if (state->room != panelRoom) {
        std::snprintf(out, static_cast<size_t>(cap),
                      "The Control Panel is in %s. Walk there before using the Fuse.",
                      map->rooms[panelRoom].id);
        return false;
    }
    const Cost cost = costUse();
    applyCost(state, cost);
    ++state->turn;
    state->fuseInstalled = true;
    state->score += cfg::kScoreFuse;
    std::snprintf(out, static_cast<size_t>(cap),
                  "Fuse installed in the Control Panel: +%d points. Lift 2 needs the Lift "
                  "Pass to run normally. [-%dE, -%dT]",
                  cfg::kScoreFuse, cost.energy, cost.time);
    resolveDefeat(state);
    return true;
}

bool useMedkit(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->passengerRescued) {
        setMessage(out, cap, "The passenger has already been treated and rescued.");
        return false;
    }
    if (!hasItem(state, IT_MEDKIT)) {
        setMessage(out, cap, "You need the Medical Kit (%) to treat the passenger.");
        return false;
    }
    const int passengerRoom = roomOfItem(map, IT_PASSENGER);
    if (state->room != passengerRoom) {
        std::snprintf(out, static_cast<size_t>(cap),
                      "The injured passenger is in %s.", map->rooms[passengerRoom].id);
        return false;
    }
    const Cost cost = costUse();
    applyCost(state, cost);
    ++state->turn;
    state->passengerRescued = true;
    state->score += cfg::kScorePassenger;
    std::snprintf(out, static_cast<size_t>(cap),
                  "The passenger is treated and can move: +%d points. [-%dE, -%dT]",
                  cfg::kScorePassenger, cost.energy, cost.time);
    resolveDefeat(state);
    return true;
}

bool useCatCarrier(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->catSecured) {
        setMessage(out, cap, "The cat is already safe inside the carrier.");
        return false;
    }
    if (!hasItem(state, IT_CAT_CARRIER)) {
        setMessage(out, cap, "The cat will not come close without the Cat Carrier (@).");
        return false;
    }
    const int catRoom = roomOfItem(map, IT_CAT);
    if (state->room != catRoom) {
        std::snprintf(out, static_cast<size_t>(cap), "The cat is hiding in %s.",
                      map->rooms[catRoom].id);
        return false;
    }
    const Cost cost = costUse();
    applyCost(state, cost);
    ++state->turn;
    state->catSecured = true;
    state->score += cfg::kScoreCat;
    std::snprintf(out, static_cast<size_t>(cap),
                  "The cat is secured in the carrier: +%d points. [-%dE, -%dT]",
                  cfg::kScoreCat, cost.energy, cost.time);
    resolveDefeat(state);
    return true;
}

bool unlockCabinet(GameState* state, const BuildingMap* map, int code, char* out,
                   int cap) {
    if (state->cabinetUnlocked) {
        setMessage(out, cap, "The security cabinet is already open.");
        return false;
    }
    const int cabinetRoom = roomOfItem(map, IT_CABINET);
    if (state->room != cabinetRoom) {
        std::snprintf(out, static_cast<size_t>(cap),
                      "The security cabinet is in %s.", map->rooms[cabinetRoom].id);
        return false;
    }
    if (clueCount(state) < 3) {
        char shown[16];
        formatClueCode(state, shown, sizeof shown);
        std::snprintf(out, static_cast<size_t>(cap),
                      "The keypad needs three digits. You have %d of 3 clues (%s).",
                      clueCount(state), shown);
        return false;
    }
    if (code != cfg::kCabinetCode) {
        setMessage(out, cap, "The keypad flashes red. Incorrect code.");
        return false;
    }
    const Cost cost = costUse();
    applyCost(state, cost);
    ++state->turn;
    state->cabinetUnlocked = true;
    state->score += cfg::kScoreCabinet;
    std::snprintf(out, static_cast<size_t>(cap),
                  "The cabinet clicks open: +%d points. A Train Key (^) is inside. "
                  "[-%dE, -%dT]",
                  cfg::kScoreCabinet, cost.energy, cost.time);
    resolveDefeat(state);
    return true;
}

bool boardTrain(GameState* state, const BuildingMap* map, char* out, int cap) {
    if (state->trainBoarded) {
        setMessage(out, cap, "You have already boarded the train.");
        return false;
    }
    const int platform = findRoom(map, "F5-R2");
    if (platform < 0) {
        setMessage(out, cap, "Map error: the boarding platform is missing.");
        return false;
    }
    if (state->room != platform) {
        std::snprintf(out, static_cast<size_t>(cap),
                      "The train waits at F5-R2 (Train Boarding Platform).");
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
    const Cost cost = costUse();
    applyCost(state, cost);
    ++state->turn;
    state->trainBoarded = true;
    state->score += cfg::kScoreTrain;
    const bool full = isFullMission(state);
    if (full) {
        state->score += cfg::kScoreVictoryBonus;
    }
    state->victory = true;
    state->over = true;
    std::snprintf(out, static_cast<size_t>(cap),
                  "You board the train with the passenger and the cat: +%d points.%s "
                  "Final score %d/%d. [-%dE, -%dT]",
                  cfg::kScoreTrain,
                  full ? " Full mission bonus: +100 points." : "", state->score,
                  cfg::kMaxScore, cost.energy, cost.time);
    return true;
}

}  // namespace

bool switches::tryTraverse(GameState* state, const BuildingMap* map, int targetRoom,
                           char* out, int cap) {
    if (targetRoom < 0 || targetRoom >= map->roomCount) {
        setMessage(out, cap, "That room does not exist.");
        return false;
    }
    const int from = state->room;
    if (targetRoom == from) {
        std::snprintf(out, static_cast<size_t>(cap), "You are already in %s.",
                      map->rooms[from].id);
        return false;
    }

    const LinkType type = findLink(map, from, targetRoom);
    if (type == LINK_NONE) {
        char exits[512];
        buildExitList(map, from, exits, static_cast<int>(sizeof exits));
        std::snprintf(out, static_cast<size_t>(cap),
                      "No direct passage from %s to %s. %s", map->rooms[from].id,
                      map->rooms[targetRoom].id, exits);
        return false;
    }
    if (type == LINK_WALL) {
        std::snprintf(out, static_cast<size_t>(cap),
                      "A solid wall separates %s and %s.", map->rooms[from].id,
                      map->rooms[targetRoom].id);
        return false;
    }
    if (type == LINK_LOCKED && !hasItem(state, IT_MASTER_KEY)) {
        std::snprintf(out, static_cast<size_t>(cap),
                      "The door between %s and %s is locked. The Master Key is required.",
                      map->rooms[from].id, map->rooms[targetRoom].id);
        return false;
    }

    bool emergency = false;
    if (type == LINK_LIFT2 && !(state->fuseInstalled && hasItem(state, IT_LIFT_PASS))) {
        if (!hasItem(state, IT_ACCESS_CARD)) {
            setMessage(out, cap,
                       "Lift 2 is unpowered. Install the Fuse and hold the Lift Pass, or "
                       "find the Emergency Access Card.");
            return false;
        }
        if (state->emergencyTrips <= 0) {
            setMessage(out, cap,
                       "The Emergency Access Card is used up. Repair Lift 2 to ride it.");
            return false;
        }
        emergency = true;
    }

    const Cost cost = costForMove(type);
    if (emergency) {
        --state->emergencyTrips;
    }
    applyCost(state, cost);
    state->room = targetRoom;
    ++state->turn;

    const Room& room = map->rooms[targetRoom];
    if (emergency) {
        std::snprintf(out, static_cast<size_t>(cap),
                      "You move to %s (%s) via %s. Emergency trip used, %d left. "
                      "[-%dE, -%dT]",
                      room.id, room.name, linkTypeName(type), state->emergencyTrips,
                      cost.energy, cost.time);
    } else {
        std::snprintf(out, static_cast<size_t>(cap), "You move to %s (%s) via %s. [-%dE, -%dT]",
                      room.id, room.name, linkTypeName(type), cost.energy, cost.time);
    }
    resolveDefeat(state);
    return true;
}

bool switches::tryCollect(GameState* state, const BuildingMap* map, const char* token,
                          char* out, int cap) {
    const int index = findItemDef(map, token);
    if (index < 0) {
        std::snprintf(out, static_cast<size_t>(cap), "There is nothing called '%s' here.",
                      token);
        return false;
    }
    const ItemDef& def = map->items[index];

    if (state->itemTaken[def.itemId]) {
        std::snprintf(out, static_cast<size_t>(cap), "You already have the %s.", def.name);
        return false;
    }
    if (def.room != state->room) {
        std::snprintf(out, static_cast<size_t>(cap), "There is no %s in %s.", def.name,
                      map->rooms[state->room].id);
        return false;
    }
    if (def.cls == CLASS_ENTITY) {
        if (def.itemId == IT_CAT) {
            setMessage(out, cap,
                       "The cat keeps its distance. You need the Cat Carrier (@).");
        } else {
            setMessage(out, cap,
                       "The passenger is too injured to walk. Use the Medical Kit (%) here.");
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

    const Cost cost = costPickup();
    applyCost(state, cost);
    ++state->turn;
    state->itemTaken[def.itemId] = true;

    switch (def.itemId) {
        case IT_ENERGY_PACK_A:
        case IT_ENERGY_PACK_B:
            addEnergy(state, def.value);
            clampResources(state);
            std::snprintf(out, static_cast<size_t>(cap),
                          "Energy Pack collected: +%d energy (%d/100). [-%dE, -%dT]",
                          def.value, state->energy, cost.energy, cost.time);
            break;
        case IT_TOOLKIT:
            addEnergy(state, def.value);
            clampResources(state);
            std::snprintf(out, static_cast<size_t>(cap),
                          "Emergency Toolkit collected: +%d energy (%d/100). [-%dE, -%dT]",
                          def.value, state->energy, cost.energy, cost.time);
            break;
        case IT_OBSERVATION_TOKEN:
            addTime(state, def.value);
            clampResources(state);
            std::snprintf(out, static_cast<size_t>(cap),
                          "Observation Token collected: +%d time (%d/100). [-%dE, -%dT]",
                          def.value, state->time, cost.energy, cost.time);
            break;
        case IT_MASTER_KEY:
            state->score += cfg::kScoreMasterKey;
            std::snprintf(out, static_cast<size_t>(cap),
                          "Master Key collected: +%d points. It opens the locked door on "
                          "floor 3. [-%dE, -%dT]",
                          cfg::kScoreMasterKey, cost.energy, cost.time);
            break;
        case IT_CLUE_FOUR:
        case IT_CLUE_TWO:
        case IT_CLUE_SEVEN: {
            if (def.itemId == IT_CLUE_FOUR) {
                state->clueFour = true;
            } else if (def.itemId == IT_CLUE_TWO) {
                state->clueTwo = true;
            } else {
                state->clueSeven = true;
            }
            state->score += cfg::kScoreClue;
            char code[16];
            formatClueCode(state, code, sizeof code);
            std::snprintf(out, static_cast<size_t>(cap),
                          "Clue digit %d found: +%d points. Code so far: %s "
                          "[-%dE, -%dT]",
                          def.value, cfg::kScoreClue, code, cost.energy, cost.time);
            break;
        }
        case IT_LIFT_PASS:
            state->score += cfg::kScoreLiftPass;
            std::snprintf(out, static_cast<size_t>(cap),
                          "Lift Pass collected: +%d points. Lift 2 runs normally once the "
                          "Fuse is installed. [-%dE, -%dT]",
                          cfg::kScoreLiftPass, cost.energy, cost.time);
            break;
        case IT_TRAIN_KEY:
            state->score += cfg::kScoreTrainKey;
            std::snprintf(out, static_cast<size_t>(cap),
                          "Train Key collected: +%d points. [-%dE, -%dT]",
                          cfg::kScoreTrainKey, cost.energy, cost.time);
            break;
        case IT_ACCESS_CARD:
            std::snprintf(out, static_cast<size_t>(cap),
                          "Emergency Access Card collected: up to %d Lift 2 trips before "
                          "repair. [-%dE, -%dT]",
                          state->emergencyTrips, cost.energy, cost.time);
            break;
        default:
            std::snprintf(out, static_cast<size_t>(cap), "%s collected. [-%dE, -%dT]",
                          def.name, cost.energy, cost.time);
            break;
    }

    resolveDefeat(state);
    return true;
}

bool switches::tryUse(GameState* state, const BuildingMap* map, ItemId target,
                      int argument, char* out, int cap) {
    switch (target) {
        case IT_FUSE:
            return useFuse(state, map, out, cap);
        case IT_MEDKIT:
            return useMedkit(state, map, out, cap);
        case IT_CAT_CARRIER:
            return useCatCarrier(state, map, out, cap);
        case IT_CABINET:
            return unlockCabinet(state, map, argument, out, cap);
        case IT_TRAIN_KEY:
            return boardTrain(state, map, out, cap);
        default: {
            const ItemDef* def = findItemById(map, target);
            std::snprintf(out, static_cast<size_t>(cap), "You cannot use the %s right now.",
                          def != nullptr ? def->name : "item");
            return false;
        }
    }
}
