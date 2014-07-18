#sudo apt-get install gcc-multilib gcc-4.7-multilib g++-4.7-multilib ia32-libs

#set(CMAKE_C_COMPILER "gcc-4.6")
#set(CMAKE_CXX_COMPILER "g++-4.6")

project( Botrix )

cmake_minimum_required( VERSION 2.6 )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra" ) # -Wno-unknown-pragmas
set( CMAKE_BUILD_TYPE Debug )

add_subdirectory ( good )

add_definitions(
                    -DVPC
                    -DPOSIX=1
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
               )

# Get files list.
file( GLOB MY_SRCS
      "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/good/include/good/*.h"
    )

include_directories(
                     "${CMAKE_CURRENT_SOURCE_DIR}"
                     "${CMAKE_CURRENT_SOURCE_DIR}/good/include"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/public"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/public/engine"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/public/tier0"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/public/tier1"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/public/game/server"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/public/mathlib"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/public/vstdlib"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/game/server"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/game/server/hl2"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/game/server/hl2mp"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/game/shared"
                     SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/../source_2013/mp/src/"
                     #"${PROJECT_BINARY_DIR}"
                   )

set( BOTRIX_LINK_LIBS
     good
     #pthread
     #rt
   )



add_library ( botrix SHARED ${MY_SRCS} )
set_target_properties( botrix PROPERTIES COMPILE_FLAGS "-m32 -march=pentium4" LINK_FLAGS "-m32 -march=pentium4" )
target_link_libraries( botrix ${BOTRIX_LINK_LIBS} )

#add_library ( gcov_botrix STATIC ${MY_SRCS} )
#set_target_properties ( gcov_botrix PROPERTIES
#                        COMPILE_FLAGS "-m32 -g --coverage -fprofile-arcs -ftest-coverage -march=pentium4"
#                        LINK_FLAGS "-m32 -march=pentium4" )
#target_link_libraries( gcov_botrix gcov ${BOTRIX_LINK_LIBS} )

