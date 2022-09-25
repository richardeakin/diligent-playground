cmake_minimum_required (VERSION 3.6)

project(TutorialFromCube CXX)
set( APP_TARGET "TutorialFromCube" )

set(SOURCE
    src/TutorialFromCube.cpp
    src/TexturedCube.cpp
)

set(INCLUDE
    src/TutorialFromCube.hpp
    src/TexturedCube.hpp
)

set(SHADERS
    assets/cube.gsh
    assets/cube.vsh
    assets/cube.psh
    assets/structures.fxh
)

set(ASSETS
    assets/raccoon.jpg
)

add_sample_app("TutorialFromCube" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

# doesn't appear to be needed, though it was added in Tutorial03_Texturing onward
# - perhaps it is if you were to run this CMakeLists.txt on its own
# target_link_libraries(Tutorial04_Instancing PRIVATE Diligent-TextureLoader)