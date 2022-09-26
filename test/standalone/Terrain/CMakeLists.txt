cmake_minimum_required (VERSION 3.6)

project(Terrain CXX)
set( APP_TARGET "Terrain" )

set(SOURCE
    src/Terrain.cpp
    ../common/src/TexturedCube.cpp
)

set(INCLUDE
    src/Terrain.hpp
    ../common/src/TexturedCube.hpp
)

set(SHADERS
    assets/cube.gsh
    assets/cube.vsh
    assets/cube.psh
    assets/rendertarget.vsh
    assets/rendertarget.psh
    assets/structures.fxh
)

set(ASSETS
    assets/raccoon.jpg
)

add_sample_app("Terrain" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
