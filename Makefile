.PHONY: clean
CPP=g++ -std=c++11

all: sff_splitter

sff_splitter: sff_splitter.o sff.o adaptors.o
	$(CPP)  -fopenmp -o sff_splitter sff_splitter.o sff.o adaptors.o

sff_splitter.o: sff_splitter.cpp sff.hpp adaptors.hpp
	$(CPP) -fopenmp -I. -c sff_splitter.cpp -fopenmp

sff.o: sff.cpp sff.hpp
	$(CPP) -I. -c sff.cpp

adaptors.o: adaptors.cpp adaptors.hpp
	$(CPP) -I. -c adaptors.cpp

clean:
	rm -f sff_splitter *.o
