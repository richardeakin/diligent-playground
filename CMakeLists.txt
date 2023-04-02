cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )


project( DiligentPlayground )

# option(BUILD_SAMPLES "Build Samples" ON)
option(BUILD_STANDALONE "Build GX" ON)
option(BUILD_GX "Build GX" OFF)

# variable_watch(CINDER_LIB_DIRECTORY)

# used for logging
include( deps/cinder/proj/cmake/utilities.cmake )

# add_subdirectory( cinder )
get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cinder/" ABSOLUTE )
# message("CINDER_PATH: ${CINDER_PATH}")

# ----
# wip Live++ support

get_filename_component( LIVEPP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/LivePP/API" ABSOLUTE )

# -------------------------------------------------------------------
# Diligent

get_filename_component(DILIGENT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/DiligentEngine" ABSOLUTE)
ci_log_i( "DILIGENT_PATH: ${DILIGENT_PATH}" )

add_subdirectory(${DILIGENT_PATH}/DiligentCore)
add_subdirectory(${DILIGENT_PATH}/DiligentTools)

if( ${BUILD_STANDALONE} )

	ci_log_i( "including standalone test apps..." )
	
	# pull in add_sample_app() and its cpp
	set(DILIGENT_BUILD_SAMPLE_BASE_ONLY ON)
	add_subdirectory(${DILIGENT_PATH}/DiligentSamples)

	# my stuff
	add_subdirectory( test/standalone )
endif()

# # -------------------------------------------------------------------
# # Cinder + Cinder-Gx

# #ci_log( debug ${CMAKE_CURRENT_LIST_LINE} "debug info" )
# if( NOT CMAKE_BUILD_TYPE )
# 	ci_log_v( "CMAKE_BUILD_TYPE not specified, defaulting to Debug" )
# 	set( CMAKE_BUILD_TYPE Debug CACHE STRING
# 		 "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel. "
# 		 FORCE
# 	)
# else()
# 	ci_log_v( "CMAKE_BUILD_TYPE set to ${CMAKE_BUILD_TYPE}" )
# endif()

# # mason things
# # set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/mason/${CMAKE_BUILD_TYPE} )
# # include( ${CMAKE_CURRENT_SOURCE_DIR}/proj/cmake/libmason.cmake )

# if( ${BUILD_GX} )
# 	# simon's block
# 	add_subdirectory( blocks/Cinder-Gx )
# 	# apps
# 	add_subdirectory( test/gx )
# endif()

# -------------------------------------------------------------------

