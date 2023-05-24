cmake_minimum_required (VERSION 3.8)

project( BasicTests CXX)
set( APP_TARGET "BasicTests" )

if( PLATFORM_WIN32 )
    option( JUNIPER_ENABLE_EDIT_AND_CONTINUE "Visual Studio Edit and Continue" OFF )
endif()

set( SOURCES
     src/BasicTests.cpp
     src/BasicTests.h
)

set( SHADERS
)

set( ASSETS ${SHADERS} )

set_source_files_properties( ${SHADERS}       PROPERTIES VS_TOOL_OVERRIDE "None" )

if( PLATFORM_MACOS )
    add_executable( ${APP_TARGET} MACOSX_BUNDLE ${SOURCES} ${ASSETS} )
elseif( PLATFORM_WIN32 )
    add_executable( ${APP_TARGET} WIN32 ${SOURCES} ${ASSETS} )
else()
    add_executable( ${APP_TARGET} ${SOURCES} ${ASSETS} )
endif()

target_include_directories( ${APP_TARGET} PUBLIC
    ${REPO_PATH}/src
    ${LIVEPP_PATH} # set in top level CMakeLists.txt
)

#add_dependencies( ${APP_TARGET} cinder )

set_target_properties( ${APP_TARGET} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
target_compile_features( ${APP_TARGET} PUBLIC cxx_std_17 )

if( JUNIPER_ENABLE_EDIT_AND_CONTINUE )
   target_compile_options( ${APP_TARGET} PUBLIC "/ZI" )
endif()

source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} ${ASSETS} )
target_include_directories( ${APP_TARGET} PRIVATE "${DILIGENT_PATH}/DiligentCore" )
if( METAL_SUPPORTED )
    target_include_directories( ${APP_TARGET} PRIVATE "${DILIGENT_PATH}/DiligentCorePro" )
endif()

target_compile_definitions( ${APP_TARGET} PRIVATE UNICODE )
target_compile_definitions( ${APP_TARGET} PRIVATE ENGINE_DLL=1 )

set_target_properties( ${APP_TARGET} PROPERTIES
    FOLDER test/juniper
)

if(D3D11_SUPPORTED)
    target_link_libraries( ${APP_TARGET} PRIVATE Diligent-GraphicsEngineD3D11-shared)
endif()
if(D3D12_SUPPORTED)
    target_link_libraries( ${APP_TARGET} PRIVATE Diligent-GraphicsEngineD3D12-shared)
endif()
if(GL_SUPPORTED)
    target_link_libraries( ${APP_TARGET} PRIVATE Diligent-GraphicsEngineOpenGL-shared)
endif()
if(VULKAN_SUPPORTED)
    target_link_libraries( ${APP_TARGET} PRIVATE Diligent-GraphicsEngineVk-shared)
endif()
if(METAL_SUPPORTED)
    target_link_libraries( ${APP_TARGET} PRIVATE Diligent-GraphicsEngineMetal-shared)
endif()
if(PLATFORM_WIN32)
    copy_required_dlls( ${APP_TARGET})
endif()

if(MSVC)
    set_target_properties( ${APP_TARGET} PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    )
endif()

if( PLATFORM_WIN32 OR PLATFORM_LINUX )
    # Copy assets to target folder
    add_custom_command( TARGET  ${APP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "\"$<TARGET_FILE_DIR:${APP_TARGET}>\"")
endif()

if(PLATFORM_MACOS AND VULKAN_LIB_PATH)
    # Configure rpath so that executables can find vulkan library
    set_target_properties( ${APP_TARGET} PROPERTIES
        BUILD_RPATH "${VULKAN_LIB_PATH}"
    )
endif()

set_source_files_properties( ${ASSETS} PROPERTIES
    VS_DEPLOYMENT_LOCATION "."
    MACOSX_PACKAGE_LOCATION "Resources"
)

# https://liveplusplus.tech/docs/documentation.html#linker_settings
set( LINK_OPTIONS
    /FUNCTIONPADMIN
    /OPT:NOREF
    /OPT:NOICF
    /DEBUG:FULL
)

target_link_options( ${APP_TARGET} PRIVATE "${LINK_OPTIONS}" )

target_include_directories( ${APP_TARGET} PUBLIC ${CINDER_INCLUDE_USER} )

set( CINDER_COMPILE_DEFINITIONS
#    WIN32
#    _DEBUG
    _LIB
    NOMINMAX
    _WIN32_WINNT=0x0601
    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS
    FT2_BUILD_LIBRARY=1
    FT_DEBUG_LEVEL_TRACE=1
)

target_compile_definitions( ${APP_TARGET} PRIVATE ${CINDER_COMPILE_DEFINITIONS} )

target_link_libraries( ${APP_TARGET} PRIVATE
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-Imgui
    Diligent-TextureLoader
    juniper
    cinder-lite
    glfw
)
