#include "physics.h"
#include "config.h"

void costForMove(LinkType type, int* costEnergy, int* costTime) {
    if (type == LINK_WINDOW) {
        *costEnergy = cfg::kWindowEnergyCost;
        *costTime = cfg::kWindowTimeCost;
    } else if (type == LINK_LIFT1 || type == LINK_LIFT2) {
        *costEnergy = cfg::kLiftEnergyCost;
        *costTime = cfg::kLiftTimeCost;
    } else if (type == LINK_STAIR) {
        *costEnergy = cfg::kStairEnergyCost;
        *costTime = cfg::kStairTimeCost;
    } else {
        // Normal doors and locked doors cost the same
        *costEnergy = cfg::kDoorEnergyCost;
        *costTime = cfg::kDoorTimeCost;
    }
}

void costInspect(int* costEnergy, int* costTime) {
    *costEnergy = cfg::kInspectEnergyCost;
    *costTime = cfg::kInspectTimeCost;
}

void costPickup(int* costEnergy, int* costTime) {
    *costEnergy = cfg::kPickupEnergyCost;
    *costTime = cfg::kPickupTimeCost;
}

void costUse(int* costEnergy, int* costTime) {
    *costEnergy = cfg::kUseEnergyCost;
    *costTime = cfg::kUseTimeCost;
}

void applyCost(int* energy, int* time, int costEnergy, int costTime) {
    *energy -= costEnergy;
    *time -= costTime;
}

void addEnergy(int* energy, int delta) {
    *energy += delta;
}

void addTime(int* time, int delta) {
    *time += delta;
}

void clampResources(int* energy, int* time) {
    if (*energy > cfg::kResourceCap) *energy = cfg::kResourceCap;
    if (*energy < 0) *energy = 0;

    if (*time > cfg::kResourceCap) *time = cfg::kResourceCap;
    if (*time < 0) *time = 0;
}

bool resolveDefeat(int* energy, int* time, bool* over, bool* victory) {
    clampResources(energy, time);
    if (*over) {
        return true;
    }
    if (*energy <= 0 || *time <= 0) {
        *over = true;
        *victory = false;
    }
    return *over;
}