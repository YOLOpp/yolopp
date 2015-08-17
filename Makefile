all: y++
	./y++
	make output


translate.o:
	g++ -c translate.cpp -Wall -std=c++11

ast.o:
	g++ -c ast.cc -Wall -std=c++11

y++: translate.o ast.o
	g++ -o y++ translate.o ast.o

output:
	g++ output.cc -o output -Wall -std=c++11 -lgmp -lgmpxx
