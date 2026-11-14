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
    cout << "Goal: rescue the passenger and the cat, then board the train at F5-R2.\n";
    cout << "Maximum score: " << cfg::kMaxScore << " points. Type HELP inside the game.\n";

    BuildingMap map;
    char error[256];
    if (!loadBuildingMap(cfg::kPathBuilding, &map, error, (int)sizeof(error))) {
        cerr << "Fatal: " << error << "\n";
        return 2;
    }

    int startRoom = findRoom(&map, "F1-R1");
    if (!validateStartingRoom(&map, startRoom)) {
        startRoom = 0;
    }

    GameState state;
    initGameState(&state, startRoom);
    logger::resetAudit();

    const char* statePath = loadSave ? cfg::kPathSaveGame : cfg::kPathInitialState;
    if (logger::loadGameStateFromFile(&state, &map, statePath, error, (int)sizeof(error))) {
        cout << "State loaded from " << statePath << "\n";
    } else {
        cerr << "Warning: " << error << "\n";
        cerr << "Starting with built-in defaults.\n";
        initGameState(&state, startRoom);
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

    int exitCode = simulator::runSimulation(&map, &state, input, echoCommands);

    if (input != stdin) {
        fclose(input);
    }
    return exitCode;
}