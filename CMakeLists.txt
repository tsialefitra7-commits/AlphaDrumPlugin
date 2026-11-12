cmake_minimum_required(VERSION 3.22)
project(AlphaDrumPlugin VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

# ALPHADRUM_CORE_ONLY=ON : compile uniquement le Core (aucun téléchargement de JUCE).
option(ALPHADRUM_CORE_ONLY "Compiler uniquement la bibliotheque Core (sans JUCE)" OFF)

# 1. Coeur logique (C++ pur)
add_subdirectory(Core)

if(NOT ALPHADRUM_CORE_ONLY)
    # JUCE, par ordre de priorité (les trois premiers fonctionnent HORS LIGNE) :
    #   1. -DJUCE_DIR_LOCAL=/chemin/vers/JUCE
    #   2. un dossier JUCE/ à la racine du projet
    #   3. un dossier JUCE/ à côté du projet (../JUCE)
    #   4. téléchargement automatique (nécessite Internet)
    set(JUCE_DIR_LOCAL "" CACHE PATH "Chemin vers une copie locale de JUCE (optionnel)")
    option(ALPHADRUM_FETCH_JUCE "Telecharger JUCE s'il est introuvable localement" ON)

    set(_juce_dir "")
    foreach(_candidate "${JUCE_DIR_LOCAL}" "${CMAKE_SOURCE_DIR}/JUCE" "${CMAKE_SOURCE_DIR}/../JUCE")
        if(_candidate AND EXISTS "${_candidate}/CMakeLists.txt" AND EXISTS "${_candidate}/modules")
            get_filename_component(_juce_dir "${_candidate}" ABSOLUTE)
            break()
        endif()
    endforeach()

    if(_juce_dir)
        message(STATUS "AlphaDrum : JUCE local utilise -> ${_juce_dir}")
        add_subdirectory("${_juce_dir}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    elseif(ALPHADRUM_FETCH_JUCE)
        message(STATUS "AlphaDrum : JUCE introuvable localement, telechargement (Internet requis)...")
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        8.0.4
            GIT_SHALLOW    TRUE)
        FetchContent_MakeAvailable(JUCE)
    else()
        message(FATAL_ERROR
            "JUCE est introuvable. Copiez le dossier JUCE (version 8) a la racine du projet "
            "(AlphaDrumPlugin/JUCE) ou passez -DJUCE_DIR_LOCAL=/chemin/vers/JUCE.")
    endif()

    # 2. Interface graphique  3. Adaptateur plugin
    add_subdirectory(UI)
    add_subdirectory(Plugin)
endif()
