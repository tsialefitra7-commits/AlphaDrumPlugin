# Coeur logique : bibliothèque statique 100% C++ standard (aucune dépendance JUCE).
add_library(AlphaDrumCore STATIC
    Source/DrumKitModel.cpp
    Source/SequencerState.cpp
    Source/SamplerEngine.cpp)

add_library(AlphaDrum::Core ALIAS AlphaDrumCore)

target_include_directories(AlphaDrumCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_compile_features(AlphaDrumCore PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(AlphaDrumCore PRIVATE /W4)
else()
    target_compile_options(AlphaDrumCore PRIVATE -Wall -Wextra)
endif()
