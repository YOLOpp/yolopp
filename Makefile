all: y++ ypp_all_source

main.o: main.cpp
	g++ -c main.cpp -Wall -std=c++11

translate.o: translate.cpp
	g++ -c translate.cpp -Wall -std=c++11

ast.o: ast.cc
	g++ -c ast.cc -Wall -std=c++11

y++: main.o translate.o ast.o y_lib.o
	g++ -o y++ translate.o ast.o main.o

y_lib.o: y_lib.cc
	g++ -c y_lib.cc -o y_lib.o -Wall -std=c++11



YPP_FILES := $(wildcard ypp/*.ypp)
OUT_FILES := $(addprefix bin/,$(notdir $(YPP_FILES:.ypp=)))

bin/%: ypp/%.ypp
	./y++ $< $@

ypp_all_source: y++ $(OUT_FILES)
