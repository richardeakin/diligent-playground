cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( CubeTest )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cinder/" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
	APP_NAME    "CubeTest"
	SOURCES     ${APP_PATH}/src/CubeTestApp.cpp
	CINDER_PATH ${CINDER_PATH}
)
