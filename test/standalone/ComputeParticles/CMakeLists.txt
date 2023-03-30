cmake_minimum_required (VERSION 3.13)

project(ComputeParticles CXX)
set(APP_TARGET "ComputeParticles")

set(SOURCE
    src/ComputeParticles.cpp
    src/Cube.cpp
)

set(INCLUDE
    src/ComputeParticles.hpp
    src/Cube.h
)

set(SHADERS
    assets/cube.psh
    assets/cube.vsh
    assets/particle.psh
    assets/particle.vsh
    assets/structures.fxh
    assets/reset_particle_lists.csh
    assets/collide_particles.csh
    assets/move_particles.csh
    assets/particles.fxh
)

set(ASSETS)

add_sample_app( ${APP_TARGET} "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}" )

# TODO: move below to common helper
set_target_properties( ${APP_TARGET} PROPERTIES CXX_STANDARD 20 )

# TODO: not working yet, look at why (could do with cmake version?)
# https://stackoverflow.com/a/52264858
set_target_properties( ${APP_TARGET} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "--mode d3d12 --adapters_dialog false" ) 

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
