CC=g++

CFLAGS=-std=c++23 -Wall -Werror -pedantic
LIBS=-lasound
HFILES=src/Fib.hpp src/Stats.hpp src/Sequencer.hpp src/Microphone.hpp src/RealTime.hpp

FILES=fib stat sequencer out/Sequencer.o out/Microphone.o

all: sequencer

fib: src/Fib.cpp $(HFILES)
	$(CC) $(CFLAGS) -o $@ $<

stat: src/Stats.cpp $(HFILES) 
	$(CC) $(CFLAGS) -o $@ $<

sequencer: src/Main.cpp out/RealTime.o out/Sequencer.o out/Microphone.o $(HFILES)
	$(CC) $(CFLAGS) -o $@ $< out/Sequencer.o out/Microphone.o out/RealTime.o

out:
	mkdir $@

out/RealTime.o: src/RealTime.cpp $(HFILES)
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $<

out/Microphone.o: src/Microphone.cpp $(HFILES)
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $< 

out/Sequencer.o: src/Sequencer.cpp $(HFILES)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(FILES)