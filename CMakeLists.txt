cmake_minimum_required(VERSION 3.22)

project(WiredMemory VERSION 0.1.0)

# ── UI strategy: embed React/Vite app via WebBrowserComponent ─────────────────
# Set to OFF to fall back to the plain paint()-based editor.
option(USE_WEB_UI "Build and embed the React UI (requires Node/npm)" ON)

# ── macOS architecture ────────────────────────────────────────────────────────
# Default to arm64 on Apple Silicon (CMAKE_HOST_SYSTEM_PROCESSOR can report
# x86_64 when CMake runs under Rosetta, producing the wrong binary).
if(NOT CMAKE_OSX_ARCHITECTURES)
    if(APPLE)
        execute_process(COMMAND uname -m OUTPUT_VARIABLE _hw_arch OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        set(_hw_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
    set(CMAKE_OSX_ARCHITECTURES "${_hw_arch}" CACHE STRING "macOS target arch" FORCE)
endif()

# ── Fetch JUCE ────────────────────────────────────────────────────────────────

set(LIB_JUCE_TAG "8.0.12")

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

juce_add_plugin(WiredMemory
    PRODUCT_NAME        "Wired Memory"
    COMPANY_NAME        "TachibanaLabs"
    BUNDLE_ID           "com.tachibanalabs.wiredmemory"
    PLUGIN_MANUFACTURER_CODE "Tclb"
    PLUGIN_CODE         "Wm01"
    FORMATS             VST3 AU Standalone
    IS_SYNTH            FALSE
    NEEDS_MIDI_INPUT    FALSE
    NEEDS_MIDI_OUTPUT   FALSE
    IS_MIDI_EFFECT      FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES     "Fx"
)

# ── Generate JuceHeader.h ─────────────────────────────────────────────────────

juce_generate_juce_header(WiredMemory)

# ── Source files ──────────────────────────────────────────────────────────────

target_sources(WiredMemory
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# ── Web UI: build React app and embed as BinaryData ───────────────────────────

if(USE_WEB_UI)
    # Require npm
    find_program(NPM_EXECUTABLE npm REQUIRED)

    set(UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ui")
    set(UI_DIST "${UI_DIR}/dist")
    set(UI_STAMP "${CMAKE_CURRENT_BINARY_DIR}/ui_build.stamp")

    # Custom target: runs `npm install` then `npm run build` whenever sources change
    file(GLOB_RECURSE UI_SOURCES CONFIGURE_DEPENDS
        "${UI_DIR}/src/*.tsx"
        "${UI_DIR}/src/*.ts"
        "${UI_DIR}/src/*.css"
        "${UI_DIR}/index.html"
        "${UI_DIR}/package.json"
    )
    add_custom_command(
        OUTPUT  "${UI_STAMP}"
        COMMAND "${NPM_EXECUTABLE}" install --prefix "${UI_DIR}"
        COMMAND "${NPM_EXECUTABLE}" run build --prefix "${UI_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${UI_STAMP}"
        DEPENDS ${UI_SOURCES}
        WORKING_DIRECTORY "${UI_DIR}"
        COMMENT "Building React plugin UI..."
    )
    add_custom_target(PluginUIBuild DEPENDS "${UI_STAMP}")

    # Glob built assets for BinaryData embedding
    # (run cmake --build once to populate dist/ before reconfiguring)
    file(GLOB_RECURSE UI_DIST_FILES CONFIGURE_DEPENDS "${UI_DIST}/*")

    if(UI_DIST_FILES)
        juce_add_binary_data(PluginUIData
            HEADER_NAME  PluginUIData.h
            NAMESPACE    PluginUIData
            SOURCES      ${UI_DIST_FILES}
        )
        add_dependencies(PluginUIData PluginUIBuild)
        set(WEB_UI_DATA_TARGET PluginUIData)
        set(WEB_BROWSER_VALUE 1)
    else()
        message(WARNING
            "USE_WEB_UI=ON but ui/dist/ is empty. "
            "Run 'npm run build' in the ui/ directory, then re-run cmake.")
        set(WEB_BROWSER_VALUE 0)
    endif()
else()
    set(WEB_BROWSER_VALUE 0)
endif()

# ── Compile definitions ───────────────────────────────────────────────────────

target_compile_definitions(WiredMemory
    PUBLIC
        JUCE_WEB_BROWSER=${WEB_BROWSER_VALUE}
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        PLUGIN_USE_WEB_UI=$<BOOL:${USE_WEB_UI}>
)

# ── Link libraries ────────────────────────────────────────────────────────────

target_link_libraries(WiredMemory
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        $<$<BOOL:${WEB_UI_DATA_TARGET}>:${WEB_UI_DATA_TARGET}>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── macOS: re-sign plugin bundles after build ────────────────────────────────
# JUCE's ad-hoc signature can become invalid after binary data is embedded.
# Without re-signing, DAWs like Ableton reject the plugin with
# "a sealed resource is missing or invalid".

if(APPLE)
    foreach(_fmt VST3 AU)
        if(TARGET WiredMemory_${_fmt})
            add_custom_command(TARGET WiredMemory_${_fmt} POST_BUILD
                COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:WiredMemory_${_fmt}>"
                COMMENT "Re-signing ${_fmt} bundle..."
            )
        endif()
    endforeach()
endif()
