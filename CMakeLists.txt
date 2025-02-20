set( CMAKE_OSX_DEPLOYMENT_TARGET 10.13 )
set( CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk )

project( botrix )

cmake_minimum_required( VERSION 3.10 )

# Build type.
# CMAKE_BUILD_TYPE gets created as what's called a cache variable on an initial configure. Don't set it here.
# Use cmake with next options:
# -DCMAKE_BUILD_TYPE=None
# -DCMAKE_BUILD_TYPE=Debug
# -DCMAKE_BUILD_TYPE=Release
# -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Compile flags.
# -g: produce debugging information in the operating system's native format.
# -m32: sets int, long, and pointer types to 32 bits, and generates code that runs on any i386 system.
# -march=pentium4: MMX, SSE and SSE2 instruction set support.
# -arch arch: i386, x86_64, ppc and ppc64 (APPLE ONLY)
# -Wl,<option>: option for linker.
# --no-undefined don't allow undefined symbols in generated shared library.
# -rdynamic: instructs the linker to add all symbols, not only used ones, to the dynamic symbol table. Needed to debug.
# -fvisibility=hidden: Default symbols visibility is hidden.

set( SHARED_COMPILE_FLAGS "-Wall -Wextra -Wno-format-security" ) # -std=c++11
if( APPLE )
    set( SHARED_COMPILE_FLAGS "${SHARED_COMPILE_FLAGS} -arch i386" )
elseif( UNIX )
    #set( SHARED_COMPILE_FLAGS "${SHARED_COMPILE_FLAGS} -m32 -march=pentium4" )
    set( SHARED_COMPILE_FLAGS "${SHARED_COMPILE_FLAGS} -m64 -march=x86-64-v2 -fPIC" ) # ld errors without x86-64-v2 and PIC
endif()

set( SHARED_LINKER_FLAGS  "-fvisibility=hidden -fpermissive -fno-rtti -fno-exceptions" )

set( SHARED_FLAGS "${SHARED_COMPILE_FLAGS} ${SHARED_LINKER_FLAGS}")

set( CMAKE_CXX_FLAGS_COVERAGE       "${SHARED_FLAGS} -DDEBUG  -D_DEBUG  -O0 -g --coverage -fprofile-arcs -ftest-coverage -lgcov" )
set( CMAKE_CXX_FLAGS_DEBUG          "${SHARED_FLAGS} -DDEBUG  -D_DEBUG  -O0 -g" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${SHARED_FLAGS} -DNDEBUG -D_NDEBUG -O2 -g" )
set( CMAKE_CXX_FLAGS_RELEASE        "${SHARED_FLAGS} -DNDEBUG -D_NDEBUG -O3" )
#-ffunction-sections -Wl,--gc-sections -O2 -falign-functions=16 -falign-loops=16" )

message( "Build type: ${CMAKE_BUILD_TYPE}" )
if( CMAKE_BUILD_TYPE MATCHES "Debug" )
    message( "Flags: ${CMAKE_CXX_FLAGS_DEBUG}" )
elseif( CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo" )
    message( "Flags: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )
else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}" )
    message( "Release flags: ${CMAKE_CXX_FLAGS}" )
endif()

# Source code.
add_subdirectory ( good )

# Source SDK 2013 sources/libs.
set( SOURCE_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013" )

# OS dependant parameters.
if( APPLE )

    set( OS_COMPILE_FLAGS -DCOMPILER_GCC -DGNUC=1 -DPOSIX=1 -DOSX=1
         # -DBOTRIX_OLD_COMMAND_COMPLETION
         -DBOTRIX_NO_COMMAND_COMPLETION
    )
    set( SOURCE_SDK_LINK_PATH "${SOURCE_SDK_PATH}/mp/src/lib/public/osx32" )
    set( OS_LINK_LIBS
        iconv c++
        "${SOURCE_SDK_LINK_PATH}/mathlib.a" # only for AngleVectors / VectorAngles!
        "${SOURCE_SDK_LINK_PATH}/tier1.a"
    )
    set( STEAMAPP_DIR "$ENV{HOME}/Library/Application\ Support/Steam/SteamApps" )
    set( STEAMCMD_DIR "$ENV{HOME}/steamcmd" )

elseif( UNIX)

    # NOTE: Valve now lets to build tier1 all by yourself
    # Including my own built tier1.a

    set( OS_COMPILE_FLAGS -DCOMPILER_GCC -DGNUC=1 -DPOSIX=1 -DLINUX=1 -D_LINUX=1 )
    set( OS_LINK_FLAGS -Wno-free-nonheap-object -rdynamic -Wl,--no-undefined -Wl,--as-needed -static-libgcc -static-libstdc++ )

    # Don't use ${SOURCE_SDK_LINK_PATH}, because those are newer ones. We need to use the earlier versions of the libs to have 20 players!
    #set( SOURCE_SDK_LINK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/linux32" )
    set( SOURCE_SDK_LINK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/linux64" )

    set(CMAKE_SHARED_LIBRARY_PREFIX "") # botrix distributes w/o lib prefix

    # While linking libpthread.a into a shared library is theoretically possible, it is a really bad idea.
    # The reason is that libpthread is part of glibc, and all parts of glibc must match exactly, or you'll
    # see strange and un-explainable crashes.
    #set( OS_LINK_LIBS /usr/lib32/libpthread.a /usr/lib32/libm.a /usr/lib32/libc.a )
    # The order of included libraries is important! mathlib.a depends on libm.a, so libm.a should be after mathlib.a!
    set( OS_LINK_LIBS
        "${SOURCE_SDK_LINK_PATH}/mathlib.a" # only for AngleVectors / VectorAngles!
        "${SOURCE_SDK_LINK_PATH}/tier1.a"
    #    "${SOURCE_SDK_LINK_PATH}/libm.a" # not needed
    )
    set( STEAMAPP_DIR "$ENV{HOME}/.steam/steam/steamapps" )
    set( STEAMCMD_DIR "$ENV{HOME}/steamcmd" )

endif()

# Defines. Don't put comments between them :).
# Not used: -DDONT_USE_VALVE_FUNCTIONS -DBOTRIX_SHOW_PERFORMANCE
# Valve not working: -DNO_MALLOC_OVERRIDE
add_definitions( ${OS_COMPILE_FLAGS}
    -DNO_MALLOC_OVERRIDE
    -DBOTRIX_SOURCE_2013
    -DBOTRIX_TF2
    -DBOTRIX_HL2DM
    -DBOTRIX_BOT_AIM_ERROR
    -DGOOD_INI_ONE_KV_SEPARATOR
    -DGOOD_INI_ONE_COMMENT_CHAR
    -DGOOD_LOG_FLUSH
    -DGOOD_LOG_USE_ENDL
    -DVECTOR
    -DGAME_DLL
    -DHL2_DLL
    -DHL2_MP
    -DHL2_EPISODIC
    -DMP_SDK
    -DVPC
)

# Get files list.
file( GLOB PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/hl2dm/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/hl2dm/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/tf2/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/tf2/*.cpp"
#    "${CMAKE_CURRENT_SOURCE_DIR}/mods/css/*.h"
#    "${CMAKE_CURRENT_SOURCE_DIR}/mods/css/*.cpp"
#    "${CMAKE_CURRENT_SOURCE_DIR}/mods/borzh/*.h"
#    "${CMAKE_CURRENT_SOURCE_DIR}/mods/borzh/*.cpp"
)

# SYSTEM is used to not show warnings/errors for included files.
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/good/include"
    SYSTEM "${SOURCE_SDK_PATH}/src/public"
    SYSTEM "${SOURCE_SDK_PATH}/src/public/engine"
    SYSTEM "${SOURCE_SDK_PATH}/src/public/tier0"
    SYSTEM "${SOURCE_SDK_PATH}/src/public/tier1"
    SYSTEM "${SOURCE_SDK_PATH}/src/public/game/server"
    SYSTEM "${SOURCE_SDK_PATH}/src/public/mathlib"
    SYSTEM "${SOURCE_SDK_PATH}/src/public/vstdlib"
    SYSTEM "${SOURCE_SDK_PATH}/src/game/server"
    SYSTEM "${SOURCE_SDK_PATH}/src/game/server/hl2"
    SYSTEM "${SOURCE_SDK_PATH}/src/game/server/hl2mp"
    SYSTEM "${SOURCE_SDK_PATH}/src/game/shared"
    SYSTEM "${SOURCE_SDK_PATH}/src/"
)

# Don't use full path dependencies in generated library.
set( CMAKE_SKIP_BUILD_RPATH true )

# Link directories.
link_directories( "${SOURCE_SDK_LINK_PATH}" )

# Link libraries.
set( PROJECT_LINK_LIBS
    #vstdlib             # looks like no need for that!
    #m
    good
    ${OS_LINK_LIBS}
    tier0
    dl                   # tier1.a needs that (dlopen etc.)
)

add_library( ${PROJECT_NAME} SHARED ${PROJECT_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${OS_LINK_FLAGS} ${PROJECT_LINK_LIBS} )

# Post-build: copy runtime files and botrix plugin to source-sdk-2013.
set( BOTRIX_LIB_PATH $<TARGET_FILE:${PROJECT_NAME}> )
set( RUNTIME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/runtime" )
set( INSTALL_PATHS
    "${SOURCE_SDK_PATH}/game/mod_hl2mp"
    "${STEAMCMD_DIR}/hl2dm/hl2mp"
    "${STEAMCMD_DIR}/tf2/tf"
    "${STEAMAPP_DIR}/common/Half-Life 2 Deathmatch/hl2mp"
    "${STEAMAPP_DIR}/common/Team Fortress 2/tf"
)

# Post-build: copy runtime files and botrix plugin to INSTALL_PATHS.
foreach( DIR ${INSTALL_PATHS} )
    if( EXISTS "${DIR}/" )
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cp -r "${RUNTIME_PATH}/*" "${DIR}/"
            COMMAND cp "${BOTRIX_LIB_PATH}" "${DIR}/addons/botrix.so"
            COMMAND /bin/echo -e "'\\e[0;32m'Copying Botrix runtime files to ${DIR}'\\e[0m'"
        )
    else()
        message( "${DIR} doesn't exist" )
    endif()
endforeach()
