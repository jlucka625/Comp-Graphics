all: Mandelbulb

Mandelbulb: main.o
	g++ main.o -o Mandelbulb -lGL -lGLU -lglut -lm

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm -rf *o Mandelbulb
