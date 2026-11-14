#include <cstdio>
#include <cstring>

#include "config.h"
#include "level.h"
#include "logger.h"
#include "simulator.h"
#include "state.h"
#include "viewer_iface.h"

int main(int argc, char** argv) {
    const char* scriptPath = nullptr;
    bool loadSave = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--load") == 0) {
            loadSave = true;
        } else if (argv[i][0] == '-') {
            std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 2;
        } else if (scriptPath == nullptr) {
            scriptPath = argv[i];
        } else {
            std::fprintf(stderr, "Only one script path may be given.\n");
            return 2;
        }
    }

    viewer::printBanner();
    std::printf("Goal: rescue the passenger and the cat, then board the train at F5-R2.\n");
    std::printf("Maximum score: %d points. Type HELP inside the game.\n", cfg::kMaxScore);

    BuildingMap map;
    char error[256];
    if (!loadBuildingMap(cfg::kPathBuilding, &map, error,
                         static_cast<int>(sizeof error))) {
        std::fprintf(stderr, "Fatal: %s\n", error);
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
    if (logger::loadGameStateFromFile(&state, &map, statePath, error,
                                      static_cast<int>(sizeof error))) {
        std::printf("State loaded from %s\n", statePath);
    } else {
        std::fprintf(stderr, "Warning: %s\n", error);
        std::fprintf(stderr, "Starting with built-in defaults.\n");
        initGameState(&state, startRoom);
    }

    std::FILE* input = stdin;
    bool echoCommands = false;
    if (scriptPath != nullptr) {
        input = std::fopen(scriptPath, "r");
        if (input == nullptr) {
            std::fprintf(stderr, "Fatal: cannot open script %s\n", scriptPath);
            return 2;
        }
        echoCommands = true;
    }

    const int exitCode = simulator::runSimulation(&map, &state, input, echoCommands);

    if (input != stdin) {
        std::fclose(input);
    }
    return exitCode;
}
