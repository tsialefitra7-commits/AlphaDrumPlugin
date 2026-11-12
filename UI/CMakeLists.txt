# Interface graphique pure : dépend uniquement de JUCE GUI (jamais du Core ni de l'audio).
#
# Bibliothèque INTERFACE : les sources sont compilées dans la cible qui la consomme
# (le plugin). C'est la manière recommandée d'utiliser les modules JUCE dans une
# sous-bibliothèque sans dupliquer leurs symboles.
add_library(AlphaDrumUI INTERFACE)
add_library(AlphaDrum::UI ALIAS AlphaDrumUI)

target_sources(AlphaDrumUI INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/FlStudioTheme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/FlPadComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/StepMatrixGrid.cpp)

target_include_directories(AlphaDrumUI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(AlphaDrumUI INTERFACE juce::juce_gui_basics)

# Ressources (polices, images) embarquées dès que des fichiers sont ajoutés dans Assets/.
file(GLOB_RECURSE ALPHADRUM_ASSETS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Fonts/*.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Fonts/*.otf
    ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Images/*.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Images/*.png)
if(ALPHADRUM_ASSETS)
    juce_add_binary_data(AlphaDrumAssets NAMESPACE AlphaDrumAssets SOURCES ${ALPHADRUM_ASSETS})
    target_link_libraries(AlphaDrumUI INTERFACE AlphaDrumAssets)
endif()
