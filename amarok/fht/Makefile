fht-test: fht.o fht-test.o
	g++ -o fht-test fht.o fht-test.o -lm -lstdc++

fht.o: fht.cpp fht.h
	g++ -c fht.cpp

fht-test.o: fht-test.cpp
	g++ -c fht-test.cpp

clean:
	rm *.o

distclean: clean
	rm fht-test

arc archive: fht-test
	tar -czf fht.tar.gz Makefile fht-test.cpp fht.cpp fht.h

vg valgrind: fht-test
	valgrind ./fht-test
