cmake_minimum_required(VERSION 3.22)

project(MyPluginName VERSION 0.1.0)

# ── macOS architecture ────────────────────────────────────────────────────────
# Default to native arch (arm64 on Apple Silicon). Override with:
#   cmake -Bbuild -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"  # universal binary
if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "macOS target arch" FORCE)
endif()

# ── Fetch JUCE ────────────────────────────────────────────────────────────────

set(LIB_JUCE_TAG "8.0.12")   # <-- update to latest tag when starting a new project

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${LIB_JUCE_TAG}
    GIT_SHALLOW    TRUE
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/Libs/JUCE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────

juce_add_plugin(MyPluginName
    PRODUCT_NAME        "My Plugin Name"           # display name in the DAW
    COMPANY_NAME        "YourName"                 # your name or studio
    BUNDLE_ID           "com.yourname.mypluginname"
    PLUGIN_MANUFACTURER_CODE "Yrnm"               # exactly 4 chars, uppercase start
    PLUGIN_CODE         "Mp01"                    # exactly 4 chars, unique per plugin
    FORMATS             VST3 AU Standalone
    IS_SYNTH            FALSE
    NEEDS_MIDI_INPUT    FALSE
    NEEDS_MIDI_OUTPUT   FALSE
    IS_MIDI_EFFECT      FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES     "Fx"
)

# ── Generate JuceHeader.h ─────────────────────────────────────────────────────

juce_generate_juce_header(MyPluginName)

# ── Source files ──────────────────────────────────────────────────────────────

target_sources(MyPluginName
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# ── Compile definitions ───────────────────────────────────────────────────────

target_compile_definitions(MyPluginName
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# ── Link libraries ────────────────────────────────────────────────────────────

target_link_libraries(MyPluginName
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
