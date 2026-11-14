#include "physics.h"
#include "config.h"

Cost costForMove(LinkType type) {
    Cost c;
    if (type == LINK_WINDOW) {
        c.energy = cfg::kWindowEnergyCost;
        c.time = cfg::kWindowTimeCost;
    } else if (type == LINK_LIFT1 || type == LINK_LIFT2) {
        c.energy = cfg::kLiftEnergyCost;
        c.time = cfg::kLiftTimeCost;
    } else if (type == LINK_STAIR) {
        c.energy = cfg::kStairEnergyCost;
        c.time = cfg::kStairTimeCost;
    } else {
        // Normal doors and locked doors cost the same
        c.energy = cfg::kDoorEnergyCost;
        c.time = cfg::kDoorTimeCost;
    }
    return c;
}

Cost costInspect() {
    Cost c = {cfg::kInspectEnergyCost, cfg::kInspectTimeCost};
    return c;
}

Cost costPickup() {
    Cost c = {cfg::kPickupEnergyCost, cfg::kPickupTimeCost};
    return c;
}

Cost costUse() {
    Cost c = {cfg::kUseEnergyCost, cfg::kUseTimeCost};
    return c;
}

void applyCost(GameState* state, const Cost& cost) {
    state->energy -= cost.energy;
    state->time -= cost.time;
}

void addEnergy(GameState* state, int delta) {
    state->energy += delta;
}

void addTime(GameState* state, int delta) {
    state->time += delta;
}

void clampResources(GameState* state) {
    if (state->energy > cfg::kResourceCap) state->energy = cfg::kResourceCap;
    if (state->energy < 0) state->energy = 0;

    if (state->time > cfg::kResourceCap) state->time = cfg::kResourceCap;
    if (state->time < 0) state->time = 0;
}

bool resolveDefeat(GameState* state) {
    clampResources(state);
    if (state->over) {
        return true;
    }
    if (state->energy <= 0 || state->time <= 0) {
        state->over = true;
        state->victory = false;
    }
    return state->over;
}