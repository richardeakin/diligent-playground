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
    ../../../src/juniper/FileWatch.h
    ../../../src/juniper/FileWatch-Monkman.hpp
)

file(GLOB_RECURSE SHADERS
    "assets/shaders/*.vsh"
    "assets/shaders/*.psh"
    "assets/shaders/*.gsh"
    "assets/shaders/*.hsh"
    "assets/shaders/*.dsh"
)

# ci_log_i( "test/Terrain| SHADERS: ${SHADERS}" )
# ci_log_i( "test/Terrain| DILIGENT_PATH: ${DILIGENT_PATH}" )

set(ASSETS
    assets/raccoon.jpg
    "${DILIGENT_PATH}/DiligentSamples/Tutorials/Tutorial08_Tessellation/assets/ps_height_1k.png"
    "${DILIGENT_PATH}/DiligentSamples/Tutorials/Tutorial08_Tessellation/assets/ps_texture_2k.png"
)

add_sample_app("${APP_TARGET}" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

set_target_properties("${APP_TARGET}" PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set( INCLUDE_USER
    "../" # for common/src
    "../../../src/" # for juniper sources
    ${LIVEPP_PATH} # set in top level CMakeLists.txt
)

target_include_directories( ${APP_TARGET} PUBLIC ${INCLUDE_USER} )
