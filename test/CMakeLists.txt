project( test-botrix )
cmake_minimum_required( VERSION 2.8 )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -m32" ) # -Wno-unknown-pragmas
#set( CMAKE_BUILD_TYPE Debug )


set ( BOTRIX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." )
set( SOURCE_SDK_2013_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../source-sdk-2013" )


# Not used: -DDONT_USE_VALVE_FUNCTIONS -DBOTRIX_SHOW_PERFORMANCE
add_definitions(
    -DDONT_USE_VALVE_FUNCTIONS=${SOURCE_SDK_2013_PATH}/mp/game/mod_hl2mp
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
    -DLINUX=1
    -D_LINUX=1
    -DPOSIX=1
    -DGNUC=1
    -DCOMPILER_GCC
)

add_subdirectory( .. "${CMAKE_CURRENT_BINARY_DIR}/botrix")
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


add_executable( ${PROJECT_NAME} ${SRC_LIST} )
target_link_libraries( ${PROJECT_NAME} dl botrix pthread ${BOTRIX_LINK_LIBS} )
set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    COMPILE_FLAGS "-m32 -march=pentium4"
    LINK_FLAGS "-m32 -march=pentium4 -rdynamic"
)
