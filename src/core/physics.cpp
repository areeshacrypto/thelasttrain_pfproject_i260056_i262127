#include "physics.h"

#include "config.h"

Cost costForMove(LinkType type) {
    switch (type) {
        case LINK_WINDOW:
            return Cost{cfg::kWindowEnergyCost, cfg::kWindowTimeCost};
        case LINK_LIFT1:
        case LINK_LIFT2:
            return Cost{cfg::kLiftEnergyCost, cfg::kLiftTimeCost};
        case LINK_STAIR:
            return Cost{cfg::kStairEnergyCost, cfg::kStairTimeCost};
        default:
            // Doors and the locked door share the ordinary movement cost.
            return Cost{cfg::kDoorEnergyCost, cfg::kDoorTimeCost};
    }
}

Cost costInspect() {
    return Cost{cfg::kInspectEnergyCost, cfg::kInspectTimeCost};
}

Cost costPickup() {
    return Cost{cfg::kPickupEnergyCost, cfg::kPickupTimeCost};
}

Cost costUse() {
    return Cost{cfg::kUseEnergyCost, cfg::kUseTimeCost};
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
    if (state->energy > cfg::kResourceCap) {
        state->energy = cfg::kResourceCap;
    }
    if (state->energy < 0) {
        state->energy = 0;
    }
    if (state->time > cfg::kResourceCap) {
        state->time = cfg::kResourceCap;
    }
    if (state->time < 0) {
        state->time = 0;
    }
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
