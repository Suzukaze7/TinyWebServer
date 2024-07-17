CC := g++
CFLAGS := -std=c++2a
SRC := $(wildcard src/*.cpp)
OBJS = $(SRC:%.cpp=%.o)
INCL := $(wildcard src/include/*)

TSRC := $(SRC) main.cpp test.cpp
$(TSRC:%.cpp=%.o): %.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

server: $(OBJS) main.o
	$(CC) $(OBJS) main.o -o server

test: $(OBJS) test.o
	$(CC) $(OBJS) test.o -o test

trun: test
	./test

.PHONY: clean

clean:
	rm -f *.o src/*.o server test