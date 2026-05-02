# Minimum CMake version required
cmake_minimum_required(VERSION 3.20)

# Project name and the languages used (C and CXX for C++)
project(FoxvoidEngine VERSION 1.0.0 LANGUAGES C CXX)

# Force modern C++ (C++20) for clean and modern syntax
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# =========================================================================
# DEPENDENCY MANAGEMENT (FetchContent)
# =========================================================================
include(FetchContent)

# Fetch Raylib directly from GitHub
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5 # Lock to version 5.5 for stability
)
FetchContent_MakeAvailable(raylib)

# Fetch nlohmann/json (The best JSON parser for C++)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# --- IMGUI & RLIMGUI (Editor UI) ---

# 1. Fetch Dear ImGui (We MUST use the 'docking' branch for a modern editor)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

# 2. Fetch rlImGui (The bridge between Raylib and ImGui)
FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG        main
)
# Tell rlImGui where to find the ImGui we just downloaded to avoid duplicates
set(IMGUI_DIR ${imgui_SOURCE_DIR}) 
FetchContent_MakeAvailable(rlImGui)

# 3. Fetch ImGuizmo (Standard library for visual manipulation in ImGui)
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(imguizmo)

# 4. Fetch ImGuiColorTextEdit (Advanced code editor for ImGui)
FetchContent_Declare(
    imgui_color_text_edit
    GIT_REPOSITORY https://github.com/BalazsJako/ImGuiColorTextEdit.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(imgui_color_text_edit)

# --- PYTHON & PYBIND11 SETUP ---

# 1. Setup Python depending on the target OS
if (CMAKE_CROSSCOMPILING AND WIN32)
    message(STATUS "Cross-compiling for Windows: Forcing Windows Python 3.11")
    
    set(Python3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/vendor/python_win/tools/include")
    set(Python3_LIBRARIES "${CMAKE_SOURCE_DIR}/vendor/python_win/tools/libs/python311.lib")
    
    set(Python3_FOUND TRUE)
    set(Python3_Development_FOUND TRUE)
    set(Python3_Interpreter_FOUND TRUE)
    set(Python3_Development.Module_FOUND TRUE)
    set(Python3_Development.Embed_FOUND TRUE)
    
    find_program(Python3_EXECUTABLE NAMES python3 python REQUIRED)

    if(NOT TARGET Python3::Python)
        add_library(Python3::Python UNKNOWN IMPORTED)
        set_target_properties(Python3::Python PROPERTIES
            IMPORTED_LOCATION "${Python3_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
        )
    endif()
    if(NOT TARGET Python3::Module)
        add_library(Python3::Module ALIAS Python3::Python)
    endif()
    if(NOT TARGET Python3::Embed)
        add_library(Python3::Embed ALIAS Python3::Python)
    endif()
else()
    find_package(Python3 COMPONENTS Interpreter Development Development.Embed REQUIRED)
endif()

# 2. Fetch pybind11 directly from GitHub
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.11.1 # Stable version compatible with modern C++
)
FetchContent_MakeAvailable(pybind11)

# --- NATIVE FILE DIALOGS ---

# Fetch Portable File Dialogs (Lightweight, single-header native OS dialogs)
FetchContent_Declare(
    pfd
    GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(pfd)

# =========================================================================
# SOURCE FILES EXPLICIT GATHERING
# =========================================================================

# 1. Grab Core Engine files (Physics, Graphics, Scripting, Audio, World, etc.)
# We explicitly list folders to avoid accidentally grabbing Editor code
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS 
    "src/audio/*.cpp"
    "src/core/*.cpp"
    "src/graphics/*.cpp"
    "src/gui/*.cpp"
    "src/physics/*.cpp"
    "src/scripting/*.cpp"
    "src/world/*.cpp"
    "src/main.cpp"
)

# 2. Grab Editor ONLY files
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS 
    "src/editor/*.cpp"
)

# 3. Grab ImGui files
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    ${imgui_color_text_edit_SOURCE_DIR}/TextEditor.cpp
)

# =========================================================================
# EDITOR BUILD TARGET (FoxvoidEngine)
# =========================================================================

# The Editor includes EVERYTHING: Engine logic + Editor UI + ImGui source code
add_executable(FoxvoidEngine ${ENGINE_SOURCES} ${EDITOR_SOURCES} ${IMGUI_SOURCES})

# Link the downloaded libraries to the editor
target_link_libraries(FoxvoidEngine PRIVATE 
    raylib 
    nlohmann_json::nlohmann_json 
    pybind11::embed
)

# Add the src/ folder to the include paths for the Editor
target_include_directories(FoxvoidEngine PRIVATE 
    src
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
    ${imguizmo_SOURCE_DIR}
    ${pfd_SOURCE_DIR}
    ${imgui_color_text_edit_SOURCE_DIR}
)

# =========================================================================
# STANDALONE BUILD TARGET (FoxvoidStandalone)
# =========================================================================

# The Standalone ONLY includes the core engine files (NO EDITOR, NO IMGUI)
add_executable(FoxvoidStandalone ${ENGINE_SOURCES})

# Link only the required libraries (No ImGui!)
target_link_libraries(FoxvoidStandalone PRIVATE 
    raylib 
    nlohmann_json::nlohmann_json 
    pybind11::embed
)

target_include_directories(FoxvoidStandalone PRIVATE 
    src
)

# Force MinGW to embed the C and C++ runtimes directly into the .exe
if (CMAKE_CROSSCOMPILING AND WIN32)
    target_link_options(FoxvoidStandalone PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# Define a macro so the C++ code knows it's running in Standalone mode
target_compile_definitions(FoxvoidStandalone PRIVATE STANDALONE_MODE)
