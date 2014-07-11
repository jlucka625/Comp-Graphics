all: raytracer

raytracer: main.o
	g++ main.o -o raytracer -lGL -lGLU -lglut -lm

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm -rf *o raytracer
