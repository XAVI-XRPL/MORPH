cmake_minimum_required(VERSION 3.22)

project(MORPH VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(JUCE)

#==============================================================================
# Engine sources are shared by the plugin and the test runner.
# (Engine code depends only on juce_core + juce_audio_basics.)

set(MORPH_ENGINE_SOURCES
    src/engine/theory/Pitch.cpp
    src/engine/theory/Scale.cpp
    src/engine/theory/KeyContext.cpp
    src/engine/theory/Chord.cpp
    src/engine/harmony/TriggerInterpreter.cpp
    src/engine/harmony/HarmonyEngine.cpp
    src/engine/harmony/StyleProfile.cpp
    src/engine/voicing/VoicingEngine.cpp
    src/engine/bass/BassEngine.cpp
    src/engine/topline/TopVoiceEngine.cpp
    src/engine/progression/Progression.cpp
    src/engine/performance/PerformanceEngine.cpp
    src/engine/EngineHost.cpp
    src/midi/MidiScheduler.cpp
    src/midi/MidiExporter.cpp
    src/state/MusicalPlaybackState.cpp
)

#==============================================================================
# Plugin

juce_add_plugin(Morph
    COMPANY_NAME "Classa Music"
    BUNDLE_ID "com.classamusic.morph"
    PLUGIN_MANUFACTURER_CODE Clam
    PLUGIN_CODE Mrp1
    FORMATS Standalone VST3 AU
    PRODUCT_NAME "MORPH"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    PRODUCES_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument
)

juce_generate_juce_header(Morph)

target_sources(Morph PRIVATE
    ${MORPH_ENGINE_SOURCES}
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp
    src/plugin/AuditionSynth.cpp
    src/ui/keyboard/KeyboardGeometry.cpp
    src/ui/keyboard/MorphKeyboard.cpp
    src/ui/radial/RadialField.cpp
    src/ui/design_system/MorphTheme.cpp
    src/ui/design_system/MaterialPainters.cpp
    src/ui/design_system/MorphLookAndFeel.cpp
    src/ui/header/HeaderControls.cpp
    src/ui/header/MorphHeader.cpp
    src/ui/controls/ActionRow.cpp
)

target_compile_definitions(Morph PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(Morph PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

#==============================================================================
# Tests

juce_add_console_app(MorphTests)

juce_generate_juce_header(MorphTests)

target_sources(MorphTests PRIVATE
    ${MORPH_ENGINE_SOURCES}
    src/ui/keyboard/KeyboardGeometry.cpp
    tests/TestHarness.cpp
    tests/theory/TheoryTests.cpp
    tests/harmony/HarmonyTests.cpp
    tests/voicing/VoicingTests.cpp
    tests/voicing/MorphVariationTests.cpp
    tests/voicing/VoiceLeadingTests.cpp
    tests/bass/BassEngineTests.cpp
    tests/topline/TopVoiceEngineTests.cpp
    tests/progression/ProgressionPlanTests.cpp
    tests/progression/PlanDumpTests.cpp
    tests/performance/PerformanceTests.cpp
    tests/midi/MidiSchedulerTests.cpp
    tests/midi/MidiExportTests.cpp
    tests/keyboard/KeyboardGeometryTests.cpp
    tests/integration/GoldenScenarioTests.cpp
    tests/integration/TruthInvariantTests.cpp
)

target_include_directories(MorphTests PRIVATE src tests)

target_compile_definitions(MorphTests PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_UNIT_TESTS=1
)

target_link_libraries(MorphTests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

enable_testing()
add_test(NAME morph_tests COMMAND MorphTests)

#==============================================================================
# UI snapshot tool (offscreen renders for visual verification)

juce_add_gui_app(MorphUISnapshot PRODUCT_NAME "MorphUISnapshot")

target_sources(MorphUISnapshot PRIVATE
    ${MORPH_ENGINE_SOURCES}
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp
    src/plugin/AuditionSynth.cpp
    src/ui/keyboard/KeyboardGeometry.cpp
    src/ui/keyboard/MorphKeyboard.cpp
    src/ui/radial/RadialField.cpp
    src/ui/design_system/MorphTheme.cpp
    src/ui/design_system/MaterialPainters.cpp
    src/ui/design_system/MorphLookAndFeel.cpp
    src/ui/header/HeaderControls.cpp
    src/ui/header/MorphHeader.cpp
    src/ui/controls/ActionRow.cpp
    tools/UISnapshotMain.cpp
)

juce_generate_juce_header(MorphUISnapshot)

target_compile_definitions(MorphUISnapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STANDALONE_APPLICATION=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JucePlugin_Build_Standalone=1
    JucePlugin_Name="MORPH"
)

target_link_libraries(MorphUISnapshot PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
