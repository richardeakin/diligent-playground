cmake_minimum_required (VERSION 3.13)

project(ComputeParticles CXX)
set(APP_TARGET "ComputeParticles")

set(SOURCE
    src/ComputeParticles.cpp
    src/Cube.cpp
    src/AppGlobal.cpp
    ../common/src/Canvas.cpp
    ../common/src/LivePP.cpp
)

set(INCLUDE
    src/ComputeParticles.hpp
    src/Cube.h
    src/AppGlobal.h
    ../common/src/Canvas.h
    ../common/src/LivePP.h
)

set(SHADERS
    assets/shaders/particles/particle_sprite.psh
    assets/shaders/particles/particle_sprite.vsh
    assets/shaders/particles/particle_solid.psh
    assets/shaders/particles/particle_solid.vsh
    assets/shaders/particles/particles.fxh
    assets/shaders/particles/structures.fxh
    assets/shaders/particles/reset_particle_lists.csh
    assets/shaders/particles/collide_particles.csh
    assets/shaders/particles/move_particles.csh
    assets/shaders/cube/cube.vsh
    assets/shaders/cube/cube.psh
    assets/shaders/canvas/canvas.vsh
    assets/shaders/canvas/canvas.psh
)

set(ASSETS)

add_sample_app( ${APP_TARGET} "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}" )

# TODO: move below to common helper
# note: FileWatch.hpp not currently compiling with C++20 (https://github.com/ThomasMonkman/filewatch/issues/39)
set_target_properties( ${APP_TARGET} PROPERTIES CXX_STANDARD 17 )

# TODO: not working yet, look at why (could do with cmake version?)
# https://stackoverflow.com/a/52264858
set_target_properties( ${APP_TARGET} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "--mode d3d12 --adapters_dialog false" ) 

set( INCLUDE_USER
    "../" # access to common/src
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
