cmake_minimum_required (VERSION 3.6)

project(ComputeParticles CXX)
set(APP_TARGET "ComputeParticles")

set(SOURCE
    src/ComputeParticles.cpp
)

set(INCLUDE
    src/ComputeParticles.hpp
)

set(SHADERS
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
