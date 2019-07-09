CXX := g++
CFLAGS := -std=c++11 -g -fPIC
LD_FLAGS := 
CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
RM := rm

simulator: ${OBJ_FILES}
	${CXX} ${LD_FLAGS} -o $@ $^

obj/%.o: src/%.cpp
	${CXX} ${CFLAGS} -c -o $@ $<

clean: 
	${RM} obj/*.o
