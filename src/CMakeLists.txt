cmake_minimum_required( VERSION 3.8 )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

project( juniper CXX )
set( LIB_TARGET "juniper" )

# file(GLOB_RECURSE SHADERS LIST_DIRECTORIES false Shaders/*.*)

file( RELATIVE_PATH JUNIPER_SRC_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" )

ci_log_i( "-----------------------------------" )
ci_log_i( "JUNIPER_SRC_DIR: ${JUNIPER_SRC_DIR}" )
ci_log_i( "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}" )
ci_log_i( "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}" )

# mason sources
set( JUNIPER_SOURCES
	juniper/AppGlfw.cpp
	juniper/AppGlfw.h
	juniper/AppEvents.cpp
	juniper/AppEvents.h
	juniper/AppGlobal.cpp
	juniper/AppGlobal.h
	juniper/Canvas.cpp
	juniper/Canvas.h
	juniper/FileWatch.h
	juniper/FileWatch-Monkman.hpp
	juniper/ImGuiImplGlfw.cpp
	juniper/ImGuiImplGlfw.h
	juniper/LivePP.cpp
	juniper/LivePP.h
	juniper/Juniper.h
	juniper/Profiler.cpp
	juniper/Profiler.h
	juniper/Solids.cpp
	juniper/Solids.h
	juniper/post/aa/FXAA.cpp
	juniper/post/aa/FXAA.h
)

set( IMGUI_SOURCES
    ${DILIGENT_DEAR_IMGUI_PATH}/backends/imgui_impl_glfw.cpp
    ${DILIGENT_DEAR_IMGUI_PATH}/backends/imgui_impl_glfw.h
)

if( PLATFORM_MACOS )
    list( APPEND JUNIPER_SOURCES juniper/SurfaceHelper.mm )
endif()

# TODO: move solid base shaders here
set( SHADERS
)

set( ASSETS ${SHADERS} )

add_library( ${LIB_TARGET} STATIC ${JUNIPER_SOURCES} ${IMGUI_SOURCES} )

set_target_properties( ${LIB_TARGET} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
target_compile_features( ${LIB_TARGET} PUBLIC cxx_std_17 )

target_compile_definitions( ${LIB_TARGET} PRIVATE UNICODE )

# creates folder layout so all .h and .cpp files live in the "src" folder, relative to their naturual folder hiearchy
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "src" FILES ${JUNIPER_SOURCES} ${ASSETS} )
# imgui files use absolute paths so add their group separately
source_group( "src/imgui" FILES ${IMGUI_SOURCES} )
#source_group( "assets" FILES ${ASSETS} )

set( INCLUDE_USER
	"." # juniper lib sources
	${LIVEPP_PATH} # set in top level CMakeLists.txt
)

target_include_directories( ${LIB_TARGET} PUBLIC ${INCLUDE_USER} )

target_include_directories( ${LIB_TARGET} PRIVATE "${DILIGENT_PATH}/DiligentCore" )
if( METAL_SUPPORTED )
    target_include_directories( ${LIB_TARGET} PRIVATE "${DILIGENT_PATH}/DiligentCorePro" )
endif()

# These add necessary includes for our library
target_compile_definitions( ${LIB_TARGET} PRIVATE ENGINE_DLL=1 )
target_link_libraries( ${LIB_TARGET} PRIVATE
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-Imgui
    Diligent-TextureLoader
    glfw
)

if( D3D11_SUPPORTED )
    target_link_libraries( ${LIB_TARGET} PRIVATE Diligent-GraphicsEngineD3D11-shared )
endif()
if( D3D12_SUPPORTED )
    target_link_libraries( ${LIB_TARGET} PRIVATE Diligent-GraphicsEngineD3D12-shared )
endif()
if( GL_SUPPORTED )
    target_link_libraries( ${LIB_TARGET} PRIVATE Diligent-GraphicsEngineOpenGL-shared )
endif()
if( VULKAN_SUPPORTED )
    target_link_libraries( ${LIB_TARGET} PRIVATE Diligent-GraphicsEngineVk-shared )
endif()
if( METAL_SUPPORTED )
    target_link_libraries( ${LIB_TARGET} PRIVATE Diligent-GraphicsEngineMetal-shared )
endif()
