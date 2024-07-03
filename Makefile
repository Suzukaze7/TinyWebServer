build:
	g++ -std=c++2a main.cpp -o server

test:
	g++ -std=c++2a -include debug.cpp test.cpp -o test && ./test