cmake_minimum_required (VERSION 3.6)

project(InstancedCubes CXX)
set( APP_TARGET "InstancedCubes" )

set(SOURCE
    src/InstancedCubes.cpp
    ../common/src/TexturedCube.cpp
    ../common/src/LivePP.cpp
)

set(INCLUDE
    src/InstancedCubes.hpp
    ../common/src/TexturedCube.hpp
    ../common/src/LivePP.h
)

set(SHADERS
    assets/cube_inst.vsh
    assets/cube.psh
)

set(ASSETS
    assets/raccoon.jpg
    assets/beaver.jpg
    assets/wombat.jpg
)

add_sample_app( ${APP_TARGET} "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}" )

# TODO: move to common helper
set_target_properties( ${APP_TARGET} PROPERTIES CXX_STANDARD 20 )

set( INCLUDE_USER
    ${LIVEPP_PATH} # set in top level CMakeLists.txt
)

target_include_directories( ${APP_TARGET} PUBLIC ${INCLUDE_USER} )

# https://liveplusplus.tech/docs/documentation.html#linker_settings
set( LINK_OPTIONS
    /FUNCTIONPADMIN
    /OPT:NOREF
    /OPT:NOICF
    /DEBUG:FULL
)

target_link_options( ${APP_TARGET} PRIVATE "${LINK_OPTIONS}" )

# doesn't appear to be needed, though it was added in Tutorial03_Texturing onward
# - perhaps it is if you were to run this CMakeLists.txt on its own
# target_link_libraries( ${APP_TARGET} PRIVATE Diligent-TextureLoader)