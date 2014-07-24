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

set( SHARED_COMPILE_FLAGS "-std=c++11 -Wall -Wextra -m32 -march=pentium4 -Wno-free-nonheap-object" )
set( SHARED_LINKER_FLAGS  "-Wl,--no-undefined -fvisibility=hidden" )

set( SHARED_FLAGS "${SHARED_COMPILE_FLAGS} ${SHARED_LINKER_FLAGS}")


set( CMAKE_CXX_FLAGS                "${SHARED_FLAGS} ${CMAKE_CXX_FLAGS}" )
set( CMAKE_CXX_FLAGS_DEBUG          "${SHARED_FLAGS} -DDEBUG  -D_DEBUG  -O0 -g -rdynamic" )
#TODO: set( CMAKE_CXX_FLAGS_GCOV           "${SHARED_FLAGS} -DDEBUG  -D_DEBUG  -O0 -g -rdynamic --coverage -fprofile-arcs -ftest-coverage -lgcov" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${SHARED_FLAGS} -DNDEBUG -D_NDEBUG -Og -rdynamic" )
set( CMAKE_CXX_FLAGS_RELEASE        "${SHARED_FLAGS} -DNDEBUG -D_NDEBUG -O3" )
#-O2 -falign-functions=16 -falign-loops=16" )

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

add_definitions(
    # Botrix defines.
    -DBOTRIX_SOURCE_2013
    -DBOTRIX_HL2DM_MOD
    -DBETA_VERSION
    -DINI_FILE_USE_ONLY_ONE_KEY_VALUE_SEPARATOR
#    -DDONT_USE_VALVE_FUNCTIONS
    # System defines.
    -DLINUX=1
    -D_LINUX=1
    -DPOSIX=1
    -DGNUC=1
    -DCOMPILER_GCC
    # Valve defines.
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

# SYSTEM is used to not show warnings/errors for included files.
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/good/include"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/public"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/public/engine"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/public/tier0"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/public/tier1"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/public/game/server"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/public/mathlib"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/public/vstdlib"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/game/server"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/game/server/hl2"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/game/server/hl2mp"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/game/shared"
    SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/"
)

set( VALVE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/lib/public/linux32" )

find_library( VALVE_MATHLIB NAMES mathlib.a     PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_VSTDLIB NAMES libvstdlib.so PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_TIER0   NAMES libtier0.so   PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_TIER1   NAMES tier1.a       PATHS ${VALVE_LIB_DIR} )
#find_library( VALVE_TIER2   NAMES tier2.a       PATHS ${VALVE_LIB_DIR} )

set( PROJECT_LINK_LIBS
    ${VALVE_MATHLIB}
    ${VALVE_TIER0}
    ${VALVE_TIER1}
    ${VALVE_TIER2}
    ${VALVE_VSTDLIB}
    dl                   # tier1.a needs that
    pthread
    good
)

set( LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/game/mod_hl2mp/addons" )

add_library( ${PROJECT_NAME} SHARED ${PROJECT_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${PROJECT_LINK_LIBS} )
#set_target_properties(
#    ${PROJECT_NAME} PROPERTIES
#    COMPILE_FLAGS "${COMPILE_FLAGS} -m32 -march=pentium4 "
#    LINK_FLAGS "${LINK_FLAGS} -m32 -march=pentium4 -rdynamic -fvisibility=hidden"
#)

#add_library ( gcov_botrix SHARED ${PROJECT_SOURCES} )
#target_link_libraries( gcov_botrix gcov ${PROJECT_LINK_LIBS} )
#set_target_properties (
#    gcov_botrix PROPERTIES
#    COMPILE_FLAGS "-m32 -g --coverage -fprofile-arcs -ftest-coverage -march=pentium4"
#    LINK_FLAGS "-m32 -march=pentium4" )
#)
