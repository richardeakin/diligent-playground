cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( HelloTriangle )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cinder/" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
    APP_NAME    "HelloTriangle"
    SOURCES     ${APP_PATH}/src/HelloTriangleApp.cpp
    CINDER_PATH ${CINDER_PATH}
)

# TODO: set APP_NAME as a variable
#target_include_directories( "HelloTriangle" PUBLIC ${ARG_INCLUDES} )
#target_link_libraries( "HelloTriangle" cinder ${ARG_LIBRARIES} )

# TODO NEXT: finish sorting through how to add diligent engine deps
get_supported_backends(DILIGENT_ENGINE_LIBRARIES)

message( "DILIGENT_ENGINE_LIBRARIES: ${DILIGENT_ENGINE_LIBRARIES}" )

target_link_libraries(HelloTriangle
PRIVATE 
    Diligent-BuildSettings
PUBLIC
    Diligent-Common
    Diligent-GraphicsTools
    # Diligent-TextureLoader
    Diligent-TargetPlatform
    Diligent-Imgui
    ${DILIGENT_ENGINE_LIBRARIES}
    # Diligent-NativeAppBase
)
