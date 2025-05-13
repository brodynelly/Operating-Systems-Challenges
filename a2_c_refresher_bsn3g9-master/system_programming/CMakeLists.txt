cmake_minimum_required(VERSION 3.5)
project(system_programming)

set(CMAKE_CXX_FLAGS "-std=c++0x -g -Wall -Werror -Wno-sign-compare")
set(CMAKE_C_FLAGS "-std=c11 -g -Wall -Wextra -Wno-sign-compare")

# Specific paths for Hellbender
set(CS4520_INCLUDE_DIRS "/cluster/pixstor/class/cs4520/include")
set(CS4520_LIB_DIRS "/cluster/pixstor/class/cs4520/lib")
include_directories(${CS4520_INCLUDE_DIRS})
link_directories(${CS4520_LIB_DIRS})

# Link runTests with what we want to test and the GTest and pthread library
add_executable(sys_prog_test test/tests.cpp src/sys_prog.c)
target_link_libraries(sys_prog_test gtest pthread)

enable_testing()
add_test(NAME    sys_prog_test
	COMMAND sys_prog_test)

file(COPY test/dat.bin DESTINATION .)
file(COPY test/offset_fun.bin DESTINATION .)
file(COPY test/out.bin DESTINATION .)
file(COPY test/WrongEndianData.bin DESTINATION .)
