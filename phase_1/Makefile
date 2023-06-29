CPPFLAGS=-Wall --pedantic -O3
CPPFLAGS+= -fopenmp
LDFLAGS=-g

debug: CPPFLAGS+= -O0
debug: CPPFLAGS+= -g -g3 -gdwarf-2
debug: CPPFLAGS+= -ftrapv
debug: CPPFLAGS+= -DDEBUG

tinyrt: tinyraytracer.o model.o main.o
	g++ $(LDFLAGS) $(CPPFLAGS) -o tinyrt tinyraytracer.o model.o main.o -lsfml-graphics -lsfml-window -lsfml-system

tinyraytracer.o: tinyraytracer.cc tinyraytracer.hh model.hh
	g++ $(CPPFLAGS) -c tinyraytracer.cc

model.o: model.cc model.hh
	g++ $(CPPFLAGS) -c model.cc

main.o: main.cc tinyraytracer.hh
	g++ $(CPPFLAGS) -c main.cc

debug: tinyrt

clean:
	rm -f tinyrt *~ *.o
