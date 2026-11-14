cmake_minimum_required(VERSION 3.16)
project(TheLastTrain CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(last_train
    src/core/main_core.cpp
    src/core/level.cpp
    src/core/simulator.cpp
    src/core/physics.cpp
    src/core/switches.cpp
    src/core/logger.cpp
    src/core/util.cpp
    src/core/viewer_iface_core.cpp
)

target_include_directories(last_train PRIVATE include)

if(NOT MSVC)
    target_compile_options(last_train PRIVATE -Wall -Wextra)
endif()
