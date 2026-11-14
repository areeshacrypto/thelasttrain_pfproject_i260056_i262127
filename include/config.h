#ifndef CONFIG_H
#define CONFIG_H

// Game constants and rules configuration
namespace cfg {

// Building dimensions
const int kFloors = 5;
const int kRoomsPerFloor = 4;
const int kMaxRooms = kFloors * kRoomsPerFloor;   // 20 rooms total
const int kMaxLinks = 64;
const int kMaxItems = 32;
const int kMaxTokens = 8;
const int kMaxLineLen = 256;
const int kIdLen = 16;
const int kNameLen = 40;
const int kItemIdLen = 24;

// Starting player resources (Time is 100 so full 520-pt run is solvable)
const int kResourceCap = 100;
const int kStartEnergy = 100;
const int kStartTime = 100;
const int kStartEmergencyTrips = 3;

// Movement and action costs
const int kDoorEnergyCost = 2;
const int kDoorTimeCost = 1;
const int kStairEnergyCost = 2;
const int kStairTimeCost = 1;
const int kWindowEnergyCost = 3;
const int kWindowTimeCost = 2;
const int kLiftEnergyCost = 4;
const int kLiftTimeCost = 3;
const int kInspectEnergyCost = 1;
const int kInspectTimeCost = 1;
const int kPickupEnergyCost = 1;
const int kPickupTimeCost = 1;
const int kUseEnergyCost = 1;
const int kUseTimeCost = 1;

// Item restore values and keypad code
const int kEnergyPackGain = 25;
const int kToolkitGain = 15;
const int kObservationTokenGain = 15;
const int kCabinetCode = 427;

// Scoring values (520 max)
const int kScoreMasterKey = 10;
const int kScoreClue = 10;
const int kScoreCabinet = 30;
const int kScoreTrainKey = 10;
const int kScorePassenger = 100;
const int kScoreCat = 100;
const int kScoreFuse = 20;
const int kScoreLiftPass = 20;
const int kScoreTrain = 100;
const int kScoreVictoryBonus = 100;
const int kMaxScore = 520;

// Log buffer limits
const int kAuditLineLen = 160;
const int kMaxAuditLines = 256;

// File paths
const char* const kPathBuilding = "./data/building.txt";
const char* const kPathInitialState = "./data/initial-state.txt";
const char* const kPathSaveGame = "./out/savegame.txt";
const char* const kPathSummary = "./out/game_summary.txt";

}  // namespace cfg

#endif