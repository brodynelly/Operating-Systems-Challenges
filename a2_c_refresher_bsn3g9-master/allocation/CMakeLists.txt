cmake_minimum_required (VERSION 3.5)
project(allocation)

set(CMAKE_CXX_FLAGS "-std=c++0x -g -Wall -Werror -Wno-sign-compare")
set(CMAKE_C_FLAGS "-std=c11 -g -Wall -Wextra -Wno-sign-compare")

# Specific paths for Hellbender
set(CS4520_INCLUDE_DIRS "/cluster/pixstor/class/cs4520/include")
set(CS4520_LIB_DIRS "/cluster/pixstor/class/cs4520/lib")
include_directories(${CS4520_INCLUDE_DIRS})
link_directories(${CS4520_LIB_DIRS})


add_executable(${PROJECT_NAME}_tester test/tests.cpp src/${PROJECT_NAME}.c)
target_link_libraries(${PROJECT_NAME}_tester gtest pthread)
