cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( HelloTriangle )
set( APP_TARGET "HelloTriangle" )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cinder" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )


message( "---------- HelloTriangle Debugging -------------------")
message( "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message( "CINDER_PATH: ${CINDER_PATH}")

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )


ci_make_app(
    APP_NAME    ${APP_TARGET}
    SOURCES     ${APP_PATH}/src/HelloTriangleApp.cpp
    CINDER_PATH ${CINDER_PATH}
)

get_supported_backends(DILIGENT_ENGINE_LIBRARIES)

# from DiligentEngine\DiligentSamples\SampleBase\CMakeLists.txt
target_link_libraries( ${APP_TARGET}
PUBLIC
    Diligent-PublicBuildSettings
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-TargetPlatform
    ${DILIGENT_ENGINE_LIBRARIES}
)

target_link_libraries( ${APP_TARGET} PRIVATE cinder-gx )

copy_required_dlls( ${APP_TARGET} )

set_target_properties( ${APP_TARGET} PROPERTIES
    FOLDER test/gx
)
