cmake_minimum_required (VERSION 3.6)

project(TutorialFromCube CXX)

set(SOURCE
    src/TutorialFromCube.cpp
)

set(INCLUDE
    src/TutorialFromCube.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
)

set(ASSETS)

add_sample_app("TutorialFromCube" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
