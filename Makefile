CC=g++

CFLAGS=-std=c++23 -Wall -Werror -pedantic
LIBS=-lasound -lfftw3 -lm
HFILES=src/Fib.hpp src/Stats.hpp src/Sequencer.hpp src/Microphone.hpp src/RealTime.hpp src/Logger.hpp src/AudioBuffer.hpp

OUTFILES=out/Logger.o out/RealTime.o out/Sequencer.o out/Microphone.o out/AudioBuffer.o
FILES=fib stat sequencer $(OUTFILES)

all: sequencer

fib: src/Fib.cpp $(HFILES)
	$(CC) $(CFLAGS) -o $@ $<

stat: src/Stats.cpp $(HFILES) 
	$(CC) $(CFLAGS) -o $@ $<

sequencer: src/Main.cpp $(OUTFILES) $(HFILES)
	$(CC) $(CFLAGS) -o $@ $< $(OUTFILES) $(LIBS)

out:
	mkdir $@

out/AudioBuffer.o: src/AudioBuffer.cpp src/AudioBuffer.hpp
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $<

out/Logger.o: src/Logger.cpp src/Logger.hpp 
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $<

out/RealTime.o: src/RealTime.cpp src/RealTime.hpp src/Logger.hpp
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $< 

out/Microphone.o: src/Microphone.cpp src/Microphone.hpp
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $< 

out/Sequencer.o: src/Sequencer.cpp $(HFILES)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(FILES)
