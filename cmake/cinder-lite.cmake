cmake_minimum_required( VERSION 3.8 )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( LIB_TARGET "cinder-lite" )

# file(GLOB_RECURSE SHADERS LIST_DIRECTORIES false Shaders/*.*)

#file( RELATIVE_PATH CINDER_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}../deps/cinder" )

ci_log_i( "-----------------------------------" )
ci_log_i( "CINDER_PATH: ${CINDER_PATH}" )

# mason sources
set( CINDER_SOURCES
    ${CINDER_PATH}/include/cinder/Cinder.h
    ${CINDER_PATH}/include/cinder/CinderAssert.h
    ${CINDER_PATH}/src/cinder/CinderAssert.cpp
    ${CINDER_PATH}/include/cinder/DataSource.h
    ${CINDER_PATH}/src/cinder/DataSource.cpp
    ${CINDER_PATH}/include/cinder/Exception.h
    ${CINDER_PATH}/src/cinder/Exception.cpp
    ${CINDER_PATH}/include/cinder/Filesystem.h
    ${CINDER_PATH}/include/cinder/Log.h
    ${CINDER_PATH}/src/cinder/Log.cpp
    ${CINDER_PATH}/src/cinder/app/Platform.cpp
    ${CINDER_PATH}/include/cinder/app/Platform.h
)

# TODO: only if windows
list( APPEND CINDER_SOURCES 
    ${CINDER_PATH}/src/cinder/app/msw/PlatformMsw.cpp
    ${CINDER_PATH}/include/cinder/app/msw/PlatformMsw.h
    ${CINDER_PATH}/src/cinder/msw/StackWalker.cpp
    ${CINDER_PATH}/include/cinder/msw/StackWalker.h
)

add_library( ${LIB_TARGET} STATIC ${CINDER_SOURCES} )

set_target_properties( ${LIB_TARGET} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
target_compile_features( ${LIB_TARGET} PUBLIC cxx_std_17 )

source_group( TREE ${CINDER_PATH} PREFIX "src" FILES ${CINDER_SOURCES} )

set( CINDER_INCLUDE_USER
	${CINDER_PATH}/include
)

target_include_directories( ${LIB_TARGET} PUBLIC ${CINDER_INCLUDE_USER} )

set( CINDER_COMPILE_DEFINITIONS
#    WIN32
#    _DEBUG
    _LIB
    NOMINMAX
    UNICODE
    _WIN32_WINNT=0x0601
    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS
    FT2_BUILD_LIBRARY=1
    FT_DEBUG_LEVEL_TRACE=1
)

target_compile_definitions( ${LIB_TARGET} PRIVATE ${CINDER_COMPILE_DEFINITIONS} )

# TODO: may need this (see cinder's platform_msw)
#    set( CINDER_STATIC_LIBS_FLAGS_DEBUG     "/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:LIBCPMT" )
#    set( MSW_PLATFORM_LIBS "Ws2_32.lib wldap32.lib shlwapi.lib OpenGL32.lib wmvcore.lib Strmiids.lib Msimg32.lib" )
