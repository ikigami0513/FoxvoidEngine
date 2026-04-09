# Minimum CMake version required
cmake_minimum_required(VERSION 3.20)

# Project name and the languages used (C and CXX for C++)
# Removed ASM as it is no longer needed without AngelScript
project(FoxvoidEngine VERSION 1.0.0 LANGUAGES C CXX)

# Force modern C++ (C++20) for clean and modern syntax
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DEPENDENCY MANAGEMENT (FetchContent)
include(FetchContent)

# Fetch Raylib directly from GitHub
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.0 # Lock to version 5.0 for stability
)
FetchContent_MakeAvailable(raylib)

# Fetch nlohmann/json (The best JSON parser for C++)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# --- PYTHON & PYBIND11 SETUP ---

# 1. Find the Python installation on the host system
# 'Development' component is required to link against libpython
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# 2. Fetch pybind11 directly from GitHub
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.11.1 # Stable version compatible with modern C++
)
FetchContent_MakeAvailable(pybind11)

# Find all .cpp files inside the src/ directory and its subdirectories
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Define the executable using the variable holding all the found files
add_executable(FoxvoidEngine ${SOURCES})

# Link the downloaded libraries to the game
# We use pybind11::embed because we are running Python INSIDE C++, 
# not building a Python extension module.
target_link_libraries(FoxvoidEngine PRIVATE 
    raylib 
    nlohmann_json::nlohmann_json 
    pybind11::embed
)

# Add the src/ folder to the include paths
target_include_directories(FoxvoidEngine PRIVATE src)
