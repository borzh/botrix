project( good_test )

cmake_minimum_required( VERSION 2.6 )

set( CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -m32 -march=pentium4 -Wno-free-nonheap-object -Wl,--no-undefined -fvisibility=hidden" )

add_definitions(
    -DINI_FILE_ONE_KV_SEPARATOR
    -DBETA_VERSION
    -DINI_FILE_ONE_KV_SEPARATOR
    -DINI_FILE_ONE_COMMENT_CHAR
)

add_subdirectory ( .. "${CMAKE_CURRENT_BINARY_DIR}/good" )

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../include" )

add_executable( ${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" )
target_link_libraries( ${PROJECT_NAME} good )
