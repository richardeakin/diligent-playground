cmake_minimum_required( VERSION 3.8 )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( LIB_TARGET "cinder-lite" )

# file(GLOB_RECURSE SHADERS LIST_DIRECTORIES false Shaders/*.*)

#file( RELATIVE_PATH CINDER_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}../deps/cinder" )

ci_log_i( "-----------------------------------" )
ci_log_i( "CINDER_PATH: ${CINDER_PATH}" )

# cinder sources
set( CINDER_SOURCES
    ${CINDER_PATH}/src/cinder/app/Platform.cpp
    ${CINDER_PATH}/include/cinder/app/Platform.h
    ${CINDER_PATH}/include/cinder/Area.h
    ${CINDER_PATH}/src/cinder/Area.cpp
    ${CINDER_PATH}/include/cinder/Buffer.h
    ${CINDER_PATH}/src/cinder/Buffer.cpp
    ${CINDER_PATH}/include/cinder/Cinder.h
    ${CINDER_PATH}/src/cinder/CinderMath.cpp
    ${CINDER_PATH}/include/cinder/CinderMath.h
    ${CINDER_PATH}/include/cinder/CinderAssert.h
    ${CINDER_PATH}/src/cinder/CinderAssert.cpp
    ${CINDER_PATH}/src/cinder/Channel.cpp
    ${CINDER_PATH}/include/cinder/Channel.h
    ${CINDER_PATH}/include/cinder/DataSource.h
    ${CINDER_PATH}/src/cinder/DataSource.cpp
    ${CINDER_PATH}/include/cinder/DataTarget.h
    ${CINDER_PATH}/src/cinder/DataTarget.cpp
    ${CINDER_PATH}/src/cinder/GeomIo.cpp
    ${CINDER_PATH}/include/cinder/GeomIo.h
    ${CINDER_PATH}/src/cinder/Surface.cpp
    ${CINDER_PATH}/include/cinder/Surface.h

    ${CINDER_PATH}/include/cinder/Stream.h
    ${CINDER_PATH}/src/cinder/Stream.cpp
    ${CINDER_PATH}/include/cinder/Exception.h
    ${CINDER_PATH}/src/cinder/Exception.cpp
    ${CINDER_PATH}/include/cinder/Filesystem.h
    ${CINDER_PATH}/include/cinder/Log.h
    ${CINDER_PATH}/src/cinder/Log.cpp
    ${CINDER_PATH}/src/cinder/Unicode.cpp
    ${CINDER_PATH}/include/cinder/Unicode.h
    ${CINDER_PATH}/src/cinder/Utilities.cpp
    ${CINDER_PATH}/include/cinder/Utilities.h
    ${CINDER_PATH}/src/cinder/Url.cpp
    ${CINDER_PATH}/include/cinder/Url.h
    ${CINDER_PATH}/src/cinder/Url.cpp
    ${CINDER_PATH}/src/cinder/UrlImplWinInet.cpp
    ${CINDER_PATH}/include/cinder/UrlImplWinInet.h

    # math things
    ${CINDER_PATH}/src/cinder/Camera.cpp
    ${CINDER_PATH}/include/cinder/Camera.h
    ${CINDER_PATH}/src/cinder/Matrix.cpp
    ${CINDER_PATH}/include/cinder/Matrix.h
    ${CINDER_PATH}/src/cinder/Ray.cpp
    ${CINDER_PATH}/include/cinder/Ray.h
    ${CINDER_PATH}/src/cinder/Sphere.cpp
    ${CINDER_PATH}/include/cinder/Sphere.h

    # Image IO
    ${CINDER_PATH}/include/cinder/ImageIo.h
    ${CINDER_PATH}/src/cinder/ImageIo.cpp
    ${CINDER_PATH}/include/cinder/ImageSourceFileWic.h
    ${CINDER_PATH}/src/cinder/ImageSourceFileWic.cpp
    ${CINDER_PATH}/include/cinder/ImageTargetFileWic.h
    ${CINDER_PATH}/src/cinder/ImageTargetFileWic.cpp
    ${CINDER_PATH}/include/cinder/ImageFileTinyExr.h
    ${CINDER_PATH}/src/cinder/ImageFileTinyExr.cpp
    ${CINDER_PATH}/include/cinder/ImageSourceFileStbImage.h
    ${CINDER_PATH}/src/cinder/ImageSourceFileStbImage.cpp
    ${CINDER_PATH}/include/cinder/ImageTargetFileStbImage.h
    ${CINDER_PATH}/src/cinder/ImageTargetFileStbImage.cpp
    ${CINDER_PATH}/include/cinder/ImageSourceFileRadiance.h
    ${CINDER_PATH}/src/cinder/ImageSourceFileRadiance.cpp

    # cinder::ip
    ${CINDER_PATH}/src/cinder/ip/fill.cpp
    ${CINDER_PATH}/include/cinder/ip/fill.h

    # linebreak
    ${CINDER_PATH}/src/linebreak/linebreak.c
    ${CINDER_PATH}/src/linebreak/linebreak.h
    ${CINDER_PATH}/src/linebreak/linebreakdata.c
    ${CINDER_PATH}/src/linebreak/linebreakdef.c
    ${CINDER_PATH}/src/linebreak/linebreakdef.h

    # tinyexr
    ${CINDER_PATH}/src/tinyexr/tinyexr.cc
    ${CINDER_PATH}/include/tinyexr/tinyexr.h
)

if( PLATFORM_WIN32 )
# TODO: only if windows
list( APPEND CINDER_SOURCES 
    ${CINDER_PATH}/src/cinder/app/msw/PlatformMsw.cpp
    ${CINDER_PATH}/include/cinder/app/msw/PlatformMsw.h
    ${CINDER_PATH}/include/cinder/msw/CinderMsw.h
    ${CINDER_PATH}/src/cinder/msw/CinderMsw.cpp
    ${CINDER_PATH}/src/cinder/msw/StackWalker.cpp # TODO: get this compiling
    ${CINDER_PATH}/include/cinder/msw/StackWalker.h
)
endif()

add_library( ${LIB_TARGET} STATIC ${CINDER_SOURCES} )

set_target_properties( ${LIB_TARGET} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
target_compile_features( ${LIB_TARGET} PUBLIC cxx_std_17 )

source_group( TREE ${CINDER_PATH} PREFIX "src" FILES ${CINDER_SOURCES} )

set( CINDER_INCLUDE_USER
)

target_include_directories( ${LIB_TARGET}
    PUBLIC
    ${CINDER_PATH}/include
    PRIVATE
    ${CINDER_PATH}/include/msw/zlib
    ${CINDER_PATH}/src/linebreak
    ${CINDER_PATH}/include/tinyexr
 )

set( CINDER_COMPILE_DEFINITIONS
#    WIN32
#    _DEBUG
    _LIB
    NOMINMAX
    UNICODE
    _UNICODE
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
