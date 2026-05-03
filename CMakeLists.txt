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
# We unconditionally fetch dependencies to avoid CMake variable scope issues, 
# but we will only build the Editor target on Desktop.

# 1. Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

# 2. Fetch rlImGui
FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG        main
)
set(IMGUI_DIR ${imgui_SOURCE_DIR}) 
FetchContent_MakeAvailable(rlImGui)

# 3. Fetch ImGuizmo (Using Populate to avoid running its broken CMakeLists.txt)
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG        master
)
FetchContent_GetProperties(imguizmo)
if(NOT imguizmo_POPULATED)
    FetchContent_Populate(imguizmo)
endif()

# 4. Fetch ImGuiColorTextEdit
FetchContent_Declare(
    imgui_color_text_edit
    GIT_REPOSITORY https://github.com/BalazsJako/ImGuiColorTextEdit.git
    GIT_TAG        master
)
FetchContent_GetProperties(imgui_color_text_edit)
if(NOT imgui_color_text_edit_POPULATED)
    FetchContent_Populate(imgui_color_text_edit)
endif()

# Fetch Portable File Dialogs
FetchContent_Declare(
    pfd
    GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(pfd)

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

elseif (ANDROID)
    message(STATUS "Cross-compiling for Android: Using BeeWare Python 3.10")
    
    # We point directly to the headers and the ARM64 shared library (.so)
    set(Python3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/vendor/python_android/include/python3.10")
    set(Python3_LIBRARIES "${CMAKE_SOURCE_DIR}/vendor/python_android/libs/arm64-v8a/libpython3.10.so")
    
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
    GIT_TAG        v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# =========================================================================
# SOURCE FILES EXPLICIT GATHERING
# =========================================================================

# 1. Grab Core Engine files (Physics, Graphics, Scripting, Audio, World, etc.)
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

# =========================================================================
# EDITOR BUILD TARGET (FoxvoidEngine) - Desktop Only
# =========================================================================
if (NOT ANDROID)
    # Grab Editor ONLY files
    file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS 
        "src/editor/*.cpp"
    )

    # Grab ImGui files
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${rlimgui_SOURCE_DIR}/rlImGui.cpp
        ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp
        ${imgui_color_text_edit_SOURCE_DIR}/TextEditor.cpp
    )

    add_executable(FoxvoidEngine ${ENGINE_SOURCES} ${EDITOR_SOURCES} ${IMGUI_SOURCES})

    target_link_libraries(FoxvoidEngine PRIVATE 
        raylib 
        nlohmann_json::nlohmann_json 
        pybind11::embed
    )

    target_include_directories(FoxvoidEngine PRIVATE 
        src
        ${imgui_SOURCE_DIR}
        ${rlimgui_SOURCE_DIR}
        ${imguizmo_SOURCE_DIR}/src
        ${pfd_SOURCE_DIR}
        ${imgui_color_text_edit_SOURCE_DIR}
    )
endif()

# =========================================================================
# STANDALONE BUILD TARGET (FoxvoidStandalone)
# =========================================================================

# On Android, the engine is compiled as a Shared Library (.so) to be loaded by Java/Kotlin
if (ANDROID)
    # The Android NDK requires native_app_glue to bridge the Activity and our C++ main()
    set(ANDROID_NATIVE_APP_GLUE "${CMAKE_SOURCE_DIR}/vendor/android_ndk/sources/android/native_app_glue")
    
    add_library(FoxvoidStandalone SHARED 
        ${ENGINE_SOURCES}
        "${ANDROID_NATIVE_APP_GLUE}/android_native_app_glue.c"
    )
    
    # Expose the app glue headers so Raylib and our code can use them
    target_include_directories(FoxvoidStandalone PRIVATE "${ANDROID_NATIVE_APP_GLUE}")
    
    # We must link standard Android libraries needed by the glue and OpenGL
    target_link_libraries(FoxvoidStandalone PRIVATE android log EGL GLESv2)
else()
    add_executable(FoxvoidStandalone ${ENGINE_SOURCES})
endif()

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
