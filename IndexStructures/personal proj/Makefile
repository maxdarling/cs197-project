TARGET_CPPS := test.cpp 
CPP_FILES := $(filter-out $(TARGET_CPPS),$(wildcard *.cpp))
OBJ_FILES := $(CPP_FILES:.cpp=.o)

#CPP_FLAGS = --std=c++17 -Wall -Werror -Wpedantic -O0 -g
CPP_FLAGS = --std=c++17 -Wall -Werror -Wpedantic -Ofast -march=native

all: test

$(OBJ_FILES) $(TARGET_CPPS:.cpp=.o): Makefile

test: $(OBJ_FILES) test.o
	g++ -o $@ $^

%.o: %.cpp
	g++ -c $(CPP_FLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f *.o test *~

