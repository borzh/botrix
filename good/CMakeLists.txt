#set(CMAKE_C_COMPILER "gcc-4.7")
#set(CMAKE_CXX_COMPILER "g++-4.7")

project( good )

cmake_minimum_required( VERSION 2.6 )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -Wall -Wextra -Wno-unknown-pragmas" )
set( CMAKE_BUILD_TYPE Debug )

add_definitions(
                    -DLINUX=1
                    -D_LINUX=1
                    -DDEBUG
                    -D_DEBUG
                    -DINI_FILE_USE_ONLY_ONE_KEY_VALUE_SEPARATOR
               )

# Get files list.
file( GLOB MY_SRCS
      "${CMAKE_CURRENT_SOURCE_DIR}/include/good/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    )

include_directories(
                     "${CMAKE_CURRENT_SOURCE_DIR}/include"
                   )

add_library( good STATIC ${MY_SRCS} )
set_target_properties( good PROPERTIES COMPILE_FLAGS "-m32 -march=pentium4" LINK_FLAGS "-m32 -march=pentium4" )

#add_library ( gcov_good STATIC ${MY_SRCS} )
#set_target_properties ( gcov_good PROPERTIES
#                        COMPILE_FLAGS "-m32 -march=pentium4-g --coverage -fprofile-arcs -ftest-coverage"
#                        LINK_FLAGS "-m32 -march=pentium4" )


add_executable( test_good "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" )
set_target_properties( test_good PROPERTIES COMPILE_FLAGS "-m32 -march=pentium4" LINK_FLAGS "-m32 -march=pentium4" )
target_link_libraries( test_good good )

#add_library ( gcov_good STATIC ${MY_SRCS} )
#set_target_properties ( gcov_good PROPERTIES
#                        COMPILE_FLAGS "-m32 -march=pentium4-g --coverage -fprofile-arcs -ftest-coverage"
#                        LINK_FLAGS "-m32 -march=pentium4" )

