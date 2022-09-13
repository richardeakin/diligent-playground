cmake_minimum_required (VERSION 3.6)

project(TutorialFromCube CXX)
set( APP_TARGET "TutorialFromCube" )

set(SOURCE
    src/TutorialFromCube.cpp
)

set(INCLUDE
    src/TutorialFromCube.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
)

set(ASSETS)

add_sample_app("TutorialFromCube" "test/standalone" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

ci_log_i( "CINDER_PATH: ${CINDER_PATH}")

# TODO: easy way to link in cinder here to use something out of it?
#find_package( cinder REQUIRED PATHS
#    "${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
#    "$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
#)

# TODO NEXT: how to know what find_package defined here?
# use these?
# target_include_directories( ${APP_TARGET} PUBLIC ${ARG_INCLUDES} )
#target_link_libraries( ${APP_TARGET} PUBLIC cinder )

# add_dependencies( ${ARG_APP_NAME} "${blockName}" )
