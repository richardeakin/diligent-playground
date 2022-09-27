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
    ../common/src/FileWatch.hpp
)

# TODO: glob everything in assets/shaders
# - also check how more complex samples include many shaders first, may already be a good way to do it
set(SHADERS
    assets/shaders/cube.vsh
    assets/shaders/cube.psh
    assets/shaders/rendertarget.vsh
    assets/shaders/rendertarget.psh
    assets/shaders/structures.fxh
)

set(ASSETS
    assets/raccoon.jpg
)

# TODO: use APP_TARGET
add_sample_app("Terrain" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

set_target_properties("Terrain" PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)