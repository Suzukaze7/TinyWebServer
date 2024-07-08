CC := g++
CFLAGS := -std=c++2a
SRC := webserver/webserver.cpp exception/exception.cpp http_conn/http_conn.cpp

SSRC := ${SRC} main.cpp
server: SSRC
	${CC} ${CFLAGS} ${SSRC} -o server

TSRC := ${SRC} test.cpp
CFLAGS += -include debug.hpp
test: ${TSRC}
	${CC} ${CFLAGS} ${TSRC} -o test