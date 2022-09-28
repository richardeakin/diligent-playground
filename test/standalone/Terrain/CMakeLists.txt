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

file(GLOB_RECURSE SHADERS
    "assets/shaders/*.vsh"
    "assets/shaders/*.psh"
    "assets/shaders/*.gsh"
    "assets/shaders/*.hsh"
    "assets/shaders/*.dsh"
)

# ci_log_i( "test/Terrain| SHADERS: ${SHADERS}" )

set(ASSETS
    assets/raccoon.jpg
)

# TODO: use APP_TARGET
add_sample_app("Terrain" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

set_target_properties("Terrain" PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)