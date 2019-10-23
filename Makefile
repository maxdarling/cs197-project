#CS 197 project initial Makefile
#Thanks to cs166 for the template makefile
	
TARGET_CPPS := test_harness.cpp
CPP_FILES := $(filter-out $(TARGET_CPPS),$(wildcard *.cpp))
OBJ_FILES := $(CPP_FILES:.cpp=.o)

#CPP_FLAGS = --std=c++17 -Wall -Werror -Wpedantic -O0 -g
CPP_FLAGS = --std=c++17 -Wall -Werror -Wpedantic -Ofast -march=native

all: test-harness

$(OBJ_FILES) $(TARGET_CPPS:.cpp=.o): Makefile

#run-bench: $(OBJ_FILES) RunBench.o
#	g++ -o $@ $^

test-harness: $(OBJ_FILES) test_harness.o
	g++ -o $@ $^

%.o: %.cpp
	g++ -c $(CPP_FLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f *.o test-harness *~
