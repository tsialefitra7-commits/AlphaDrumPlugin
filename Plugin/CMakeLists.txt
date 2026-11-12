# Adaptateur : le pont DAW <-> Core <-> UI.
set(ALPHADRUM_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND ALPHADRUM_FORMATS AU)
endif()

juce_add_plugin(AlphaDrum
    PRODUCT_NAME                "AlphaDrum"
    COMPANY_NAME                "Alpha"
    PLUGIN_MANUFACTURER_CODE    Alph
    PLUGIN_CODE                 Adrm
    FORMATS                     ${ALPHADRUM_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(AlphaDrum PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(AlphaDrum PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AlphaDrum
    PRIVATE
        AlphaDrum::Core
        AlphaDrum::UI
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
