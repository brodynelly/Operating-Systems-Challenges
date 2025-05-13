###############################################################################
# University of Missouri
# CMP_SC 4520/7520
# Fall 2024
# A3.5 Threading Assignment
# By Jim Ries (RiesJ@missouri.edu)
#
# Makefile
###############################################################################

# Macros
CC = gcc
CPP = g++
CFLAGS = -Wall -Werror -c -g -fPIC
CPPFLAGS = -Wall -Werror -c -g -fPIC
LDFLAGS = -ljimrfile -ljimrpics -L. -pthread

# Default target
default: test

# Rules
%.o : %.c 
	@echo Compiling $^ 
	@$(CC) $(CFLAGS) $^

# Targets
test : test.o Student.o
	@echo Linking $@ 
	@$(CPP) $^ $(LDFLAGS) -o $@

clean :
	@rm -rf *.o
	@rm -rf test
	@rm -rf *.jpg
