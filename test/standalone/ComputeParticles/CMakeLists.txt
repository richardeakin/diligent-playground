cmake_minimum_required (VERSION 3.13)

project(ComputeParticles CXX)
set(APP_TARGET "ComputeParticles")

set(SOURCE
    src/ComputeParticles.cpp
    src/SolidsOriginal.cpp
    ../../../src/juniper/AppGlobal.cpp
    # ../../../src/juniper/Solids.cpp
    ../../../src/juniper/Canvas.cpp
    ../../../src/juniper/LivePP.cpp 
    ../../../src/juniper/Profiler.cpp
    ../../../src/juniper/post/aa/FXAA.cpp
)

set(INCLUDE
    src/ComputeParticles.hpp
    src/SolidsOriginal.h
    ../../../src/juniper/AppGlobal.h
    # ../../../src/juniper/Solids.h
    ../../../src/juniper/Canvas.h
    ../../../src/juniper/LivePP.h
    ../../../src/juniper/FileWatch.h
    ../../../src/juniper/FileWatch-Monkman.hpp
    ../../../src/juniper/Profiler.h
    ../../../src/juniper/post/aa/FXAA.h
)

set(SHADERS
    assets/shaders/particles/particle_sprite.psh
    assets/shaders/particles/particle_sprite.vsh
    assets/shaders/particles/particle_solid.psh
    assets/shaders/particles/particle_solid.vsh
    assets/shaders/particles/particles.fxh
    assets/shaders/particles/structures.fxh
    assets/shaders/particles/reset_particle_lists.csh
    assets/shaders/particles/interact_particles.csh
    assets/shaders/particles/move_particles.csh
    assets/shaders/solids/solid.vsh
    assets/shaders/solids/solid.psh
    assets/shaders/solids/solid.fxh
    assets/shaders/canvas/canvas.vsh
    assets/shaders/canvas/canvas.psh
    assets/shaders/canvas/canvasRaymarcher.psh
    assets/shaders/Quaternion.hlsl
    assets/shaders/post/post_process.vsh
    assets/shaders/post/post_process.psh
    assets/shaders/post/downsample.psh
    assets/shaders/post/aa/fxaa.vsh
    assets/shaders/post/aa/fxaa.psh
    assets/shaders/post/aa/FXAA3_11.h
)

set(ASSETS)

add_sample_app( ${APP_TARGET} "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}" )

# TODO: move below to common helper
# note: FileWatch.hpp not currently compiling with C++20 (https://github.com/ThomasMonkman/filewatch/issues/39)
set_target_properties( ${APP_TARGET} PROPERTIES CXX_STANDARD 17 )
set_target_properties( ${APP_TARGET} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "--mode d3d12 --adapters_dialog false" ) 

set( INCLUDE_USER
    "../" # for common/src
    "../../../src/" # for juniper sources
    ${LIVEPP_PATH} # set in top level CMakeLists.txt
    ../../../deps/cinder/include
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

# TODO: remove once glm is fully in, or keep this test's sources separate
target_compile_definitions( ${APP_TARGET} PRIVATE CINDER_DISABLED )
