#include <cstdio>
#include <cstring>
#include <iostream>
#include "config.h"
#include "level.h"
#include "logger.h"
#include "simulator.h"
#include "state.h"
#include "viewer_iface.h"

using namespace std;

int main(int argc, char** argv) {
    const char* scriptPath = nullptr;
    bool loadSave = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--load") == 0) {
            loadSave = true;
        } else if (argv[i][0] == '-') {
            cerr << "Unknown option: " << argv[i] << "\n";
            return 2;
        } else if (scriptPath == nullptr) {
            scriptPath = argv[i];
        } else {
            cerr << "Only one script path may be given.\n";
            return 2;
        }
    }

    viewer::printBanner();
    cout << "Your job: rescue the injured passenger and the cat, then board the last\n";
    cout << "train at F5-R2. Every action costs energy and time - both start at 100,\n";
    cout << "and if either one reaches 0 the night is over. Maximum score: "
         << cfg::kMaxScore << " points.\n";

    // The building map, stored as parallel arrays.
    char roomId[cfg::kMaxRooms][cfg::kIdLen];
    char roomName[cfg::kMaxRooms][cfg::kNameLen];
    int roomFloor[cfg::kMaxRooms];
    int roomSlot[cfg::kMaxRooms];
    int roomGridRow[cfg::kMaxRooms];
    int roomGridCol[cfg::kMaxRooms];
    int roomCount = 0;

    int linkA[cfg::kMaxLinks];
    int linkB[cfg::kMaxLinks];
    LinkType linkType[cfg::kMaxLinks];
    int linkCount = 0;

    char itemCode[cfg::kMaxItems][cfg::kItemIdLen];
    char itemSymbol[cfg::kMaxItems];
    char itemName[cfg::kMaxItems][cfg::kNameLen];
    int itemRoom[cfg::kMaxItems];
    ItemClass itemClass[cfg::kMaxItems];
    ItemId itemAs[cfg::kMaxItems];
    int itemValue[cfg::kMaxItems];
    int itemCount = 0;

    int grid[cfg::kFloors][cfg::kRoomsPerFloor];

    char error[256];
    if (!loadBuildingMap(cfg::kPathBuilding, roomId, roomName, roomFloor, roomSlot,
                         roomGridRow, roomGridCol, &roomCount, linkA, linkB, linkType,
                         &linkCount, itemCode, itemSymbol, itemName, itemRoom, itemClass,
                         itemAs, itemValue, &itemCount, grid, error, (int)sizeof(error))) {
        cerr << "Fatal: " << error << "\n";
        return 2;
    }

    int startRoom = findRoom(roomId, roomCount, "F1-R1");
    if (!validateStartingRoom(roomCount, startRoom)) {
        startRoom = 0;
    }

    // The game state, stored as plain variables plus one bool array.
    int energy = cfg::kStartEnergy;
    int time = cfg::kStartTime;
    int score = 0;
    int room = startRoom;
    int emergencyTrips = cfg::kStartEmergencyTrips;
    int turn = 0;
    bool clueFour = false;
    bool clueTwo = false;
    bool clueSeven = false;
    bool fuseInstalled = false;
    bool cabinetUnlocked = false;
    bool passengerRescued = false;
    bool catSecured = false;
    bool trainBoarded = false;
    bool itemTaken[IT_COUNT] = {};
    bool over = false;
    bool victory = false;

    logger::resetAudit();

    const char* statePath = loadSave ? cfg::kPathSaveGame : cfg::kPathInitialState;
    if (logger::loadGameStateFromFile(&energy, &time, &score, &room, &turn, &emergencyTrips,
                                      &fuseInstalled, &cabinetUnlocked, &passengerRescued,
                                      &catSecured, &trainBoarded, &clueFour, &clueTwo,
                                      &clueSeven, itemTaken, &over, &victory,
                                      roomId, roomCount, itemCode, itemName, itemSymbol,
                                      itemAs, itemCount, statePath, error, (int)sizeof(error))) {
        cout << "State loaded from " << statePath << "\n";
    } else {
        cerr << "Warning: " << error << "\n";
        cerr << "Starting with built-in defaults.\n";
    }

    FILE* input = stdin;
    bool echoCommands = false;
    if (scriptPath != nullptr) {
        input = fopen(scriptPath, "r");
        if (input == nullptr) {
            cerr << "Fatal: cannot open script " << scriptPath << "\n";
            return 2;
        }
        echoCommands = true;
    }

    int exitCode = simulator::runSimulation(&energy, &time, &score, &room, &emergencyTrips, &turn,
                                            itemTaken, &clueFour, &clueTwo, &clueSeven,
                                            &fuseInstalled, &cabinetUnlocked, &passengerRescued,
                                            &catSecured, &trainBoarded, &over, &victory,
                                            roomId, roomName, roomFloor, linkA, linkB, linkType,
                                            linkCount, roomCount, itemCode, itemSymbol, itemName,
                                            itemRoom, itemClass, itemAs, itemValue, itemCount,
                                            grid, input, echoCommands);

    if (input != stdin) {
        fclose(input);
    }
    return exitCode;
}