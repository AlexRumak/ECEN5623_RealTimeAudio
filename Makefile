CC=g++

CFLAGS=-std=c++23 -Wall -Werror -pedantic
HFILES=src/Fib.hpp src/Stats.hpp src/Sequencer.hpp

FILES=fib stat sequencer out/Sequencer.o

all: sequencer

fib: src/Fib.cpp $(HFILES)
	$(CC) $(CFLAGS) -o $@ $<

stat: src/Stats.cpp $(HFILES) 
	$(CC) $(CFLAGS) -o $@ $<

sequencer: src/Main.cpp out/Sequencer.o $(HFILES)
	$(CC) $(CFLAGS) -o $@ $< out/Sequencer.o

out:
	mkdir $@

out/Sequencer.o: src/Sequencer.cpp $(HFILES) out
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(FILES)