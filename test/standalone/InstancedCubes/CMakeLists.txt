cmake_minimum_required (VERSION 3.6)

project(InstancedCubes CXX)
set( APP_TARGET "InstancedCubes" )

set(SOURCE
    src/InstancedCubes.cpp
    ../common/src/TexturedCube.cpp
)

set(INCLUDE
    src/InstancedCubes.hpp
    ../common/src/TexturedCube.hpp
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

add_sample_app("InstancedCubes" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

# doesn't appear to be needed, though it was added in Tutorial03_Texturing onward
# - perhaps it is if you were to run this CMakeLists.txt on its own
# target_link_libraries(Tutorial04_Instancing PRIVATE Diligent-TextureLoader)