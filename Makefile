D ?= 1
SRC := main.cpp

CXX := g++
CFLAGS := -std=c++2a -Wall
LDFLAGS := 

ifeq ($(D), 1)
	CFLAGS += -g -fsanitize=address
	LDFLAGS += -lasan
else
	CFLAGS += -o3 -DNDEBUG -s
endif

SRC += $(wildcard src/*.cpp)
OBJS = $(SRC:%.cpp=%.o)
INCL := $(wildcard src/include/*)

server: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o server

$(OBJS): %.o: %.cpp $(INCL)
	$(CXX) $(CFLAGS) -c $< -o $@

run: server
	./server

.PHONY: clean
clean:
	rm -f *.o src/*.o server

C ?= 200
T ?= 5

bench:
	webbench -c $(C) -t $(T) -2 http://localhost:8080/