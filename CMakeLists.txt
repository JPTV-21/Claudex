cmake_minimum_required(VERSION 3.16)

# Keep the executable self-contained instead of requiring the VC++ runtime.
# This must be set before project() so it applies while the toolchain is
# initialized.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(ClaudeX VERSION 1.1.1 LANGUAGES CXX)

set(CLAUDEX_AUTHOR "Wind_Blood")

option(CLAUDEX_BUILD_TESTS "Build the unit tests (run them with ctest)" ON)

# Windows resource compiler, used to embed the application icon.
if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ftxui/CMakeLists.txt")
    message(FATAL_ERROR "Missing third_party/ftxui. Restore the bundled dependency.")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json.hpp")
    message(FATAL_ERROR "Missing third_party/nlohmann/json.hpp. Restore the bundled dependency.")
endif()

set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS_FUZZER OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_MODULES OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(FTXUI_QUIET ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/ftxui)

set(DEFAULT_CONFIG_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/default-config.json")
set(DEFAULT_CONFIG_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/generated/default_config.hpp")
if(NOT EXISTS "${DEFAULT_CONFIG_SOURCE}")
    message(FATAL_ERROR "Missing resources/default-config.json.")
endif()
file(READ "${DEFAULT_CONFIG_SOURCE}" DEFAULT_CONFIG_CONTENT)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(WRITE "${DEFAULT_CONFIG_HEADER}"
    "// Generated from resources/default-config.json. Do not edit.\n"
    "#pragma once\n\n"
    "namespace claudex_defaults {\n"
    "inline constexpr const char kConfigJson[] = R\"CLAUDEX_DEFAULT(\n"
    "${DEFAULT_CONFIG_CONTENT}\n"
    ")CLAUDEX_DEFAULT\";\n"
    "}\n"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${DEFAULT_CONFIG_SOURCE}")

set(DEFAULT_THEME_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/default-theme.json")
set(DEFAULT_THEME_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/generated/default_theme.hpp")
if(NOT EXISTS "${DEFAULT_THEME_SOURCE}")
    message(FATAL_ERROR "Missing resources/default-theme.json.")
endif()
file(READ "${DEFAULT_THEME_SOURCE}" DEFAULT_THEME_CONTENT)
file(WRITE "${DEFAULT_THEME_HEADER}"
    "// Generated from resources/default-theme.json. Do not edit.\n"
    "#pragma once\n\n"
    "namespace claudex_defaults {\n"
    "inline constexpr const char kThemeJson[] = R\"CLAUDEX_THEME(\n"
    "${DEFAULT_THEME_CONTENT}\n"
    ")CLAUDEX_THEME\";\n"
    "}\n"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${DEFAULT_THEME_SOURCE}")

# --- Helpers ---------------------------------------------------------------

function(claudex_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/utf-8 /W4>)
    else()
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>)
    endif()
endfunction()

# Statically link the MinGW runtime so executables are single portable files.
function(claudex_link_static target)
    if(WIN32 AND NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(${target} PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
            # Drop the symbol table from release binaries; it is not needed
            # at runtime and roughly halves the MinGW executable size.
            $<$<CONFIG:Release>:-s>
        )
    endif()
endfunction()

# --- Libraries -------------------------------------------------------------

# Everything that does not need a terminal UI: text and path handling, config
# loading, Claude discovery and process launching.
add_library(claudex_core STATIC
    src/text_util.cpp
    src/platform.cpp
    src/path_resolver.cpp
    src/config_types.cpp
    src/config_json.cpp
    src/config_bootstrap.cpp
    src/config_manager.cpp
    src/claude_finder.cpp
    src/launcher.cpp
)
target_include_directories(claudex_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
claudex_apply_warnings(claudex_core)

# Menu markup, screen layout, and key matching built on FTXUI.
add_library(claudex_ui STATIC
    src/menu_markup.cpp
    src/screens.cpp
    src/grid_menu.cpp
    src/key_events.cpp
)
target_link_libraries(claudex_ui PUBLIC claudex_core ftxui::ftxui)
claudex_apply_warnings(claudex_ui)

# --- Launcher executable -----------------------------------------------------

add_executable(ClaudeX main.cpp)

if(WIN32)
    set(CLAUDEX_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/claudex.ico")
    set(CLAUDEX_ICON_RC "${CMAKE_CURRENT_BINARY_DIR}/generated/claudex.rc")
    set(CLAUDEX_VERSION_COMMA
        "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/claudex.rc.in"
        "${CLAUDEX_ICON_RC}"
        @ONLY)
    set_source_files_properties("${CLAUDEX_ICON_RC}"
        PROPERTIES OBJECT_DEPENDS "${CLAUDEX_ICON_PATH}")
    target_sources(ClaudeX PRIVATE "${CLAUDEX_ICON_RC}")
endif()

target_link_libraries(ClaudeX PRIVATE claudex_ui)
claudex_apply_warnings(ClaudeX)
claudex_link_static(ClaudeX)

# --- Tests -------------------------------------------------------------------

if(CLAUDEX_BUILD_TESTS)
    enable_testing()

    file(GLOB CLAUDEX_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    add_executable(claudex_tests ${CLAUDEX_TEST_SOURCES})
    target_include_directories(claudex_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    # Lets the tests read the shipped theme presets from the source tree.
    target_compile_definitions(claudex_tests PRIVATE
        CLAUDEX_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(claudex_tests PRIVATE claudex_ui)
    claudex_apply_warnings(claudex_tests)
    claudex_link_static(claudex_tests)

    add_test(NAME unit COMMAND claudex_tests)
endif()
