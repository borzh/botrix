project( good )

cmake_minimum_required( VERSION 2.6 )

#add_definitions( -DINI_FILE_USE_ONLY_ONE_KEY_VALUE_SEPARATOR )

# Get files list.
file( GLOB PROJECT_SOURCES
      "${CMAKE_CURRENT_SOURCE_DIR}/include/good/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    )

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_library( ${PROJECT_NAME} STATIC ${PROJECT_SOURCES} )
target_link_libraries( ${PROJECT_NAME} pthread )

add_executable( test_good "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" )
target_link_libraries( test_good ${PROJECT_NAME} )

#add_library ( gcov_good STATIC ${PROJECT_SOURCES} )
#set_target_properties ( gcov_good PROPERTIES
#                        COMPILE_FLAGS "-m32 -march=pentium4-g --coverage -fprofile-arcs -ftest-coverage"
#                        LINK_FLAGS "-m32 -march=pentium4" )
