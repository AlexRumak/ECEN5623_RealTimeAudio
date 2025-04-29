CC=g++

CFLAGS=-std=c++23 -Wall -Werror -pedantic
LIBS=-lasound
HFILES=src/Fib.hpp src/Stats.hpp src/Sequencer.hpp src/Microphone.hpp


FILES=fib stat sequencer out/Sequencer.o out/Microphone.o

all: sequencer

fib: src/Fib.cpp $(HFILES)
	$(CC) $(CFLAGS) -o $@ $<

stat: src/Stats.cpp $(HFILES) 
	$(CC) $(CFLAGS) -o $@ $<

sequencer: src/Main.cpp out/Sequencer.o out/Microphone.o $(HFILES)
	$(CC) $(CFLAGS) -o $@ $< out/Sequencer.o out/Microphone.o

out:
	mkdir $@

out/Microphone.o: src/Microphone.cpp
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $< 

out/Sequencer.o: src/Sequencer.cpp $(HFILES) out
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(FILES)