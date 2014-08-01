project( botrix )

cmake_minimum_required( VERSION 2.6 )


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
# -Wl,<option>: option for linker.
# --no-undefined don't allow undefined symbols in generated shared library.
# -rdynamic: instructs the linker to add all symbols, not only used ones, to the dynamic symbol table. Needed to debug.
# -fvisibility=hidden: Default symbols visibility is hidden.

set( SHARED_COMPILE_FLAGS "-std=c++11 -Wall -Wextra -m32 -march=pentium4 -Wno-format-security -Wno-free-nonheap-object" )
set( SHARED_LINKER_FLAGS  "-Wl,--no-undefined -fvisibility=hidden" )

set( SHARED_FLAGS "${SHARED_COMPILE_FLAGS} ${SHARED_LINKER_FLAGS}")


set( CMAKE_CXX_FLAGS                "${SHARED_FLAGS} ${CMAKE_CXX_FLAGS}" )
set( CMAKE_CXX_FLAGS_DEBUG          "${SHARED_FLAGS} -DDEBUG  -D_DEBUG  -O0 -g -rdynamic" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${SHARED_FLAGS} -DNDEBUG -D_NDEBUG -g -O2 -rdynamic" )
set( CMAKE_CXX_FLAGS_RELEASE        "${SHARED_FLAGS} -DNDEBUG -D_NDEBUG -O3" )
#-O2 -falign-functions=16 -falign-loops=16" )
#TODO: set( CMAKE_CXX_FLAGS_GCOV           "${SHARED_FLAGS} -DDEBUG  -D_DEBUG  -O0 -g -rdynamic --coverage -fprofile-arcs -ftest-coverage -lgcov" )

message( "Build type: ${CMAKE_BUILD_TYPE}" )
if( CMAKE_BUILD_TYPE MATCHES "None" )
    message( "Flags: ${CMAKE_CXX_FLAGS}" )
elseif( CMAKE_BUILD_TYPE MATCHES "Debug" )
    message( "Flags: ${CMAKE_CXX_FLAGS_DEBUG}" )
elseif( CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo" )
    message( "Flags: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )
elseif( CMAKE_BUILD_TYPE MATCHES "Release" )
    message( "Flags: ${CMAKE_CXX_FLAGS_RELEASE}" )
endif()


# Source code.
add_subdirectory ( good )

# Defines. Don't put comments between them :).
# Not used: -DDONT_USE_VALVE_FUNCTIONS -DBOTRIX_SHOW_PERFORMANCE
add_definitions(
    -DGOOD_INI_ONE_KV_SEPARATOR
    -DGOOD_INI_ONE_COMMENT_CHAR
    -DGOOD_LOG_FLUSH
    -DGOOD_LOG_USE_ENDL
    -DBOTRIX_SOURCE_2013
    -DBOTRIX_HL2DM_MOD
    -DLINUX=1
    -D_LINUX=1
    -DPOSIX=1
    -DGNUC=1
    -DCOMPILER_GCC
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
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/css/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/css/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/borzh/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/borzh/*.cpp"
)

set( SOURCE_SDK_2013_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013" )

# SYSTEM is used to not show warnings/errors for included files.
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/good/include"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/public"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/public/engine"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/public/tier0"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/public/tier1"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/public/game/server"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/public/mathlib"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/public/vstdlib"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/game/server"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/game/server/hl2"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/game/server/hl2mp"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/game/shared"
    SYSTEM "${SOURCE_SDK_2013_PATH}/mp/src/"
)


# Don't use full path dependencies in generated library.
set( CMAKE_SKIP_BUILD_RPATH true )

# Link libraries.
set( SOURCE_SDK_2013_LINK_PATH "${SOURCE_SDK_2013_PATH}/mp/src/lib/public/linux32" )
link_directories( ${SOURCE_SDK_2013_LINK_PATH} )

set( PROJECT_LINK_LIBS
    ${SOURCE_SDK_2013_LINK_PATH}/mathlib.a
    ${SOURCE_SDK_2013_LINK_PATH}/tier1.a
    libtier0.so
    libvstdlib.so
    dl                   # tier1.a needs that
    good
)


# Link Botrix plugin library.
add_library( ${PROJECT_NAME} SHARED ${PROJECT_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${PROJECT_LINK_LIBS} )


# Post-build: copy runtime files and botrix plugin to source-sdk-2013.
get_target_property( BOTRIX_LIB_PATH ${PROJECT_NAME} LOCATION )
set( RUNTIME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/runtime" )
set( SDK_2013_MOD_PATH "${SOURCE_SDK_2013_PATH}/mp/game/mod_hl2mp" )
set( HL2DM_MOD_PATH "$ENV{HOME}/.steam/steam/SteamApps/common/Half-Life 2 Deathmatch/hl2mp" )
set( SRCDS_HL2DM_MOD_PATH "$ENV{HOME}/steamcmd/hl2dm/hl2mp" )

# Post-build: copy runtime files and botrix plugin to Source SDK 2013 mp mod.
if( EXISTS "${SRCDS_HL2DM_MOD_PATH}/" )
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND cp -r "${RUNTIME_PATH}/*" "${SDK_2013_MOD_PATH}/"
        COMMAND cp "${BOTRIX_LIB_PATH}" "${SDK_2013_MOD_PATH}/addons/botrix.so"
        COMMAND /bin/echo -e "'\\e[0;32m'Copying Botrix runtime files to Source SDK 2013: ${SDK_2013_MOD_PATH}'\\e[0m'"
    )
endif()

# Post-build: copy runtime files and botrix plugin to Source Dedicated Server.
if( EXISTS "${SRCDS_HL2DM_MOD_PATH}/" )
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND cp -r "${RUNTIME_PATH}/*" "${SRCDS_HL2DM_MOD_PATH}/"
        COMMAND cp "${BOTRIX_LIB_PATH}" "${SRCDS_HL2DM_MOD_PATH}/addons/botrix.so"
        COMMAND /bin/echo -e "'\\e[0;32m'Copying Botrix runtime files to SRCDS: ${SRCDS_HL2DM_MOD_PATH}'\\e[0m'"
    )
endif()

# Post-build: copy runtime files and botrix plugin to HL2DM.
if( EXISTS "${HL2DM_MOD_PATH}/" )
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND cp -r "${RUNTIME_PATH}/*" "${HL2DM_MOD_PATH}/"
        COMMAND cp "${BOTRIX_LIB_PATH}" "${HL2DM_MOD_PATH}/addons/botrix.so"
        COMMAND /bin/echo -e "'\\e[0;32m'Copying Botrix runtime files to HL2DM: ${HL2DM_MOD_PATH}'\\e[0m'"
    )
endif()
