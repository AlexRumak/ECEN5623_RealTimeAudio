CC=g++

CFLAGS=-std=c++23 -Wall -Werror -pedantic
HFILES=Fib.hpp Stats.hpp Sequencer.hpp

PRODUCT=fib stat sequencer Sequencer.o

all: sequencer

fib: Fib.cpp $(HFILES)
	$(CC) $(CFLAGS) -o $@ $<

stat: Stats.cpp $(HFILES) 
	$(CC) $(CFLAGS) -o $@ $<

sequencer: Main.cpp Sequencer.o $(HFILES)
	$(CC) $(CFLAGS) -o $@ $< Sequencer.o

Sequencer.o: Sequencer.cpp $(HFILES)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PRODUCT)