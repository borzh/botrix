#sudo apt-get install gcc-multilib gcc-4.7-multilib g++-4.7-multilib ia32-libs

#set(CMAKE_C_COMPILER "gcc-4.7")
#set(CMAKE_CXX_COMPILER "g++-4.7")

project( Botrix )

cmake_minimum_required( VERSION 2.6 )

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -std=c++11 -Wall -Wextra -O0 -g -ggdb3" ) #  -Wno-unknown-pragmas
#set( CMAKE_BUILD_TYPE Debug )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -std=c++11 -Wall -Wextra -Wno-free-nonheap-object -O2 -falign-functions=16 -falign-loops=16" ) #  -Wno-unknown-pragmas
set( CMAKE_BUILD_TYPE Release )

add_subdirectory ( good )

add_definitions(
    -DVPC
    -DPOSIX=1
#    -D_DEBUG=1
#    -DDEBUG=1
    -D_NDEBUG=1
    -DNDEBUG=1
    -DGNUC=1
    -DCOMPILER_GCC
    -DLINUX=1
    -D_LINUX=1
    -DDEBUG
    -D_DEBUG
    -DVECTOR
    -DGAME_DLL
    -DHL2_DLL
    -DHL2_MP
    -DHL2_EPISODIC
    -DMP_SDK
    -DBOTRIX_SOURCE_2013
    -DBOTRIX_HL2DM_MOD
    -DINI_FILE_USE_ONLY_ONE_KEY_VALUE_SEPARATOR
    -DDONT_USE_VALVE_FUNCTIONS
)

# Get files list.
file( GLOB MY_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/hl2dm/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/hl2dm/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/css/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/css/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/borzh/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/borzh/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/good/include/good/*.h"
)

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
    #"${PROJECT_BINARY_DIR}"
)

set( VALVE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/lib/public/linux32" )

find_library( VALVE_MATHLIB NAMES mathlib.a     PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_VSTDLIB NAMES libvstdlib.so PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_TIER0   NAMES libtier0.so   PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_TIER1   NAMES tier1.a       PATHS ${VALVE_LIB_DIR} )
#find_library( VALVE_TIER2   NAMES tier2.a       PATHS ${VALVE_LIB_DIR} )

set( BOTRIX_LINK_LIBS
    good
    ${VALVE_MATHLIB}
    ${VALVE_TIER0}
    ${VALVE_TIER1}
    ${VALVE_TIER2}
    ${VALVE_VSTDLIB}
)

set( OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/game/mod_hl2mp/addons" )

add_library( botrix SHARED ${MY_SRCS} )
target_link_libraries( botrix ${BOTRIX_LINK_LIBS} )
set_target_properties(
    botrix PROPERTIES
    COMPILE_FLAGS "-m32 -march=pentium4"
    LINK_FLAGS "-m32 -march=pentium4 -rdynamic"
# -fvisibility=hidden"
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

#add_library ( gcov_botrix SHARED ${MY_SRCS} )
#target_link_libraries( gcov_botrix gcov ${BOTRIX_LINK_LIBS} )
#set_target_properties (
#    gcov_botrix PROPERTIES
#    COMPILE_FLAGS "-m32 -g --coverage -fprofile-arcs -ftest-coverage -march=pentium4"
#    LINK_FLAGS "-m32 -march=pentium4" )
#    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
#)
