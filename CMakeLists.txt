cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )


project( DiligentPlayground )

option(BUILD_SAMPLES "Build Samples" ON)

# variable_watch(CINDER_LIB_DIRECTORY)

# -------------------------------------------------------------------
# Diligent

# add_subdirectory(blocks/Cinder-Gx/third_party/DiligentCore)
# add_subdirectory(blocks/Cinder-Gx/third_party/DiligentTools)

# -------------------------------------------------------------------
# Cinder

include( cinder/proj/cmake/utilities.cmake )

#ci_log_i( "testing log info." )
#ci_log( debug ${CMAKE_CURRENT_LIST_LINE} "debug info" )
ci_log_v( "cinder path: ${CINDER_PATH_FULL}" )
if( NOT CMAKE_BUILD_TYPE )
	ci_log_v( "CMAKE_BUILD_TYPE not specified, defaulting to Debug" )
	set( CMAKE_BUILD_TYPE Debug CACHE STRING
		 "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel. "
		 FORCE
	)
else()
	ci_log_v( "CMAKE_BUILD_TYPE set to ${CMAKE_BUILD_TYPE}" )
endif()

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/mason/${CMAKE_BUILD_TYPE} )

# include( ${CMAKE_CURRENT_SOURCE_DIR}/proj/cmake/libmason.cmake )

add_subdirectory( cinder )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cinder/" ABSOLUTE )
message("CINDER_PATH: ${CINDER_PATH}")

add_subdirectory( blocks/Cinder-Gx )

# -------------------------------------------------------------------
# apps

add_subdirectory( test )
