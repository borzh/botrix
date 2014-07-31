project( test-botrix )
cmake_minimum_required( VERSION 2.8 )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -m32" ) # -Wno-unknown-pragmas
set( CMAKE_BUILD_TYPE Debug )


set ( BOTRIX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../botrix" )
set( SOURCE_SDK_2013_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013" )


# Not used: -DDONT_USE_VALVE_FUNCTIONS -DBOTRIX_SHOW_PERFORMANCE
add_definitions(
    -DDONT_USE_VALVE_FUNCTIONS=${SOURCE_SDK_2013_PATH}/mp/game/mod_hl2mp
    -DGOOD_LOG_FLUSH
    -DGOOD_LOG_USE_ENDL
    -DBOTRIX_SOURCE_2013
    -DBOTRIX_HL2DM_MOD
    -DBETA_VERSION
    -DGOOD_LOG_USE_ENDL
    -DINI_FILE_ONE_KV_SEPARATOR
    -DINI_FILE_ONE_COMMENT_CHAR
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

add_subdirectory( ../botrix "${CMAKE_CURRENT_BINARY_DIR}/botrix")

aux_source_directory( . SRC_LIST )

include_directories(
    SYSTEM "${BOTRIX_DIR}"
    SYSTEM "${BOTRIX_DIR}/good/include"
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
    #"${PROJECT_BINARY_DIR}"
)

set( VALVE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../source-sdk-2013/mp/src/lib/public/linux32" )

find_library( VALVE_MATHLIB NAMES mathlib.a     PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_VSTDLIB NAMES libvstdlib.so PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_TIER0   NAMES libtier0.so   PATHS ${VALVE_LIB_DIR} )
find_library( VALVE_TIER1   NAMES tier1.a       PATHS ${VALVE_LIB_DIR} )
#find_library( VALVE_TIER2   NAMES tier2.a       PATHS ${VALVE_LIB_DIR} )

set( BOTRIX_LINK_LIBS
    ${VALVE_MATHLIB}
    ${VALVE_TIER0}
    ${VALVE_TIER1}
    ${VALVE_TIER2}
    ${VALVE_VSTDLIB}
)


add_executable( ${PROJECT_NAME} ${SRC_LIST} )
target_link_libraries( ${PROJECT_NAME} dl botrix pthread ${BOTRIX_LINK_LIBS} )
set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    COMPILE_FLAGS "-m32 -march=pentium4"
    LINK_FLAGS "-m32 -march=pentium4 -rdynamic"
)
