CXX := g++
CFLAGS := -std=c++11
LD_FLAGS := -std=c++11
CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))

simulator: ${OBJ_FILES}
	${CXX} ${LD_FLAGS} -o $@ $^

obj/%.o: src/%.cpp
	${CXX} ${CFLAGS} -c -o $@ $<
