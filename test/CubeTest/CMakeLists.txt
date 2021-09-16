cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( CubeTest )
set( APP_TARGET "CubeTest" )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cinder/" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
    APP_NAME    ${APP_TARGET}
	SOURCES     ${APP_PATH}/src/CubeTestApp.cpp
	CINDER_PATH ${CINDER_PATH}
)

# TODO: can I do this from the parent CMakeLists.txt?
# - can probably use a helper function for all my tests that adds this after passing in the target name
set_target_properties( ${APP_TARGET} PROPERTIES
    FOLDER test
)