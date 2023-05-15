cmake_minimum_required( VERSION 3.8 )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

project( Juniper CXX )

# file(GLOB_RECURSE SHADERS LIST_DIRECTORIES false Shaders/*.*)

file( RELATIVE_PATH JUNIPER_SRC_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" )

ci_log_i( "-----------------------------------" )
ci_log_i( "JUNIPER_SRC_DIR: ${JUNIPER_SRC_DIR}" )
ci_log_i( "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}" )
ci_log_i( "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}" )

# mason sources
list( APPEND JUNIPER_SOURCES
	juniper/AppGlobal.cpp
	juniper/AppGlobal.h
	juniper/Juniper.h
)

set( SHADERS
)

set( ASSETS ${SHADERS} )

add_library( Juniper STATIC ${JUNIPER_SOURCES} )

set_target_properties( Juniper PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
target_compile_features( Juniper PUBLIC cxx_std_17 )

source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${ASSETS} )
