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

target_compile_definitions( Juniper PRIVATE UNICODE )

# TODO: this fixes the sources so they match the natural folder structure but still want them to end up in a src/ subtree - how?
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${JUNIPER_SOURCES} ${ASSETS} )

target_include_directories( Juniper PRIVATE "${DILIGENT_PATH}/DiligentCore" )
if( METAL_SUPPORTED )
    target_include_directories( Juniper PRIVATE "${DILIGENT_PATH}/DiligentCorePro" )
endif()

# These add necessary includes for our library
target_compile_definitions( Juniper PRIVATE ENGINE_DLL=1 )
target_link_libraries( Juniper PRIVATE
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-Imgui
    glfw
)

if( D3D11_SUPPORTED )
    target_link_libraries( Juniper PRIVATE Diligent-GraphicsEngineD3D11-shared )
endif()
if( D3D12_SUPPORTED )
    target_link_libraries( Juniper PRIVATE Diligent-GraphicsEngineD3D12-shared )
endif()
if( GL_SUPPORTED )
    target_link_libraries( Juniper PRIVATE Diligent-GraphicsEngineOpenGL-shared )
endif()
if( VULKAN_SUPPORTED )
    target_link_libraries( Juniper PRIVATE Diligent-GraphicsEngineVk-shared )
endif()
if( METAL_SUPPORTED )
    target_link_libraries( Juniper PRIVATE Diligent-GraphicsEngineMetal-shared )
endif()
