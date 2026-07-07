cmake_minimum_required(VERSION 3.22)

# =============================================================================
# Project metadata
# Keep VERSION in sync with getApplicationVersion() / ProjectInfo::versionString
# =============================================================================
project(LiveEqualizer
    VERSION     1.0.0
    DESCRIPTION "System-Wide Windows Audio Equalizer — JUCE 8 + Equalizer APO IPC"
    LANGUAGES   CXX
)

# =============================================================================
# C++ standard — C++20 required for structured bindings, std::format-ready code
# =============================================================================
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Feeds clangd / VS Code IntelliSense

# =============================================================================
# JUCE 8 — point at a local installation or let FetchContent download it.
#
# Option A (default): use the local JUCE 8 already on this machine.
# Option B: pass -DJUCE_PATH=<path> on the cmake command line to override.
# Option C: set JUCE_PATH="" and uncomment the FetchContent block below to
#           auto-download JUCE 7.0.9 (requires internet on first configure).
# =============================================================================
set(JUCE_PATH "C:/Users/jerom/Downloads/juce-8.0.13-windows/JUCE"
    CACHE PATH "Path to the local JUCE SDK directory")

if(EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(STATUS "Using local JUCE at: ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    message(STATUS "Local JUCE not found — downloading via FetchContent (requires internet)")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.9
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# =============================================================================
# Application target
# =============================================================================
juce_add_gui_app(LiveEqualizer
    PRODUCT_NAME "System Audio Equalizer"
    VERSION      "1.0.0"
    COMPANY_NAME "Jerome"
    BUNDLE_ID    "com.jerome.audioequalizer"
)

# Auto-generates JuceHeader.h in the build directory (includes all linked modules)
juce_generate_juce_header(LiveEqualizer)

# =============================================================================
# Source files
# NOTE: AudioEqualizer.cpp / biquadFilter.cpp are intentionally excluded.
#       DSP work is now delegated to Equalizer APO via file-based IPC.
# =============================================================================
target_sources(LiveEqualizer PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
)

# =============================================================================
# Compiler definitions
# =============================================================================
target_compile_definitions(LiveEqualizer PRIVATE
    JUCE_WEB_BROWSER=0              # Removes WebKit/CEF dependency
    JUCE_USE_CURL=0                 # Removes libcurl dependency
    JUCE_STRICT_REFCOUNTEDPOINTER=1

    # Inject version strings into ProjectInfo:: namespace (used by Main.cpp)
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:LiveEqualizer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:LiveEqualizer,JUCE_VERSION>"
)

# =============================================================================
# Module dependencies
# Only GUI modules are needed — no audio I/O modules required because all
# DSP processing is delegated to Equalizer APO.
# =============================================================================
target_link_libraries(LiveEqualizer PRIVATE
    juce::juce_core          # String, File, DBG, Timer, etc.
    juce::juce_events        # MessageManager, async callbacks
    juce::juce_graphics      # Graphics, Font, Colour, Image
    juce::juce_gui_basics    # Component, Slider, Label, DocumentWindow

    juce::juce_recommended_config_flags   # /O2 in Release, /Od in Debug
    juce::juce_recommended_warning_flags  # -Wall equivalent per compiler
)

# =============================================================================
# Install rule — copies the Release .exe next to a generated desktop shortcut.
# Usage:  cmake --install build --config Release --prefix "C:/Users/jerom/Desktop"
# =============================================================================
install(TARGETS LiveEqualizer
    RUNTIME DESTINATION .
    CONFIGURATIONS Release
)
