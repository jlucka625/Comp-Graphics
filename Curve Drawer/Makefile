all: hw3

hw3: main.o
	g++ main.o -o hw3 -lGL -lGLU -lglut -lm

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm -rf *o hw3
