CC := g++
CFLAGS := -std=c++2a
SRC := webserver/webserver.cpp http_conn/http_conn.cpp exception/exception.cpp utils/utils.cpp
NSRC := ${SRC} thread_pool/thread_pool.hpp logger/logger.hpp type/type.h

server: ${NSRC} main.cpp
	${CC} ${CFLAGS} ${SRC} main.cpp -o server

test: ${NSRC} test.cpp
	${CC} ${CFLAGS} ${SRC} test.cpp -o test

trun: test
	./test