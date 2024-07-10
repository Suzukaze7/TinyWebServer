CC := g++
CFLAGS := -std=c++2a
SRC := webserver/webserver.cpp exception/exception.cpp http_conn/http_conn.cpp
NSRC := ${SRC} thread_pool/thread_pool.hpp

SSRC := ${SRC} main.cpp
server: ${NSRC}
	${CC} ${CFLAGS} ${SSRC} -o server

TSRC := ${SRC} test.cpp
test: ${NSRC}
	${CC} ${CFLAGS} ${TSRC} -o test