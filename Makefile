#CS 197 project initial Makefile
#Thanks to cs166 for the template makefile
#TODO: Have it make when .hh file is edited
	
TARGET_CPPS := test_harness.cpp adaptive_hash.cpp 
CPP_FILES := $(filter-out $(TARGET_CPPS),$(wildcard *.cpp))
OBJ_FILES := $(CPP_FILES:.cpp=.o)

#CPP_FLAGS = --std=c++17 -Wall -Werror -Wpedantic -O0 -g
CPP_FLAGS = --std=c++17 -Wall -Werror -Wpedantic -Ofast -march=native

all: test-harness 

$(OBJ_FILES) $(TARGET_CPPS:.cpp=.o): Makefile

#adapt-hash: $(OBJ_FILES) adaptive_hash.o
#	g++ -o $@ $^

test-harness: $(OBJ_FILES) test_harness.o adaptive_hash.o
	g++ -o $@ $^

%.o: %.cpp
	g++ -c $(CPP_FLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f *.o test-harness *~
