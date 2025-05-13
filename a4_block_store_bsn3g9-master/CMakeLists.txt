cmake_minimum_required (VERSION 2.8)
project(assignment4)

set(CMAKE_C_FLAGS "-std=c11 -Wall -Wextra -Wshadow -Werror -D_XOPEN_SOURCE=500 -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wshadow -Werror -D_XOPEN_SOURCE=500 -Wno-sign-compare")

# Specific paths for Hellbender
set(CS4520_INCLUDE_DIRS "/cluster/pixstor/class/cs4520/include")
set(CS4520_LIB_DIRS "/cluster/pixstor/class/cs4520/lib")
include_directories(${CS4520_INCLUDE_DIRS})
link_directories(${CS4520_LIB_DIRS})

include_directories("${PROJECT_SOURCE_DIR}/include")

# build a dynamic library called libblock_store.so
# note that the prefix lib will be automatically added in the filename.
add_library(block_store SHARED src/block_store.c src/bitmap.c)

# make an executable
add_executable(${PROJECT_NAME}_test test/tests.cpp)
target_compile_definitions(${PROJECT_NAME}_test PRIVATE)
target_link_libraries(${PROJECT_NAME}_test gtest pthread block_store)
