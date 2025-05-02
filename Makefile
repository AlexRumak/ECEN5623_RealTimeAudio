CC=g++

CFLAGS=-std=c++23 -Wall -Werror -pedantic
DEBUG=-g -fsanitize=address
LIBS=-lasound -lfftw3 -lm -lncurses
HFILES=src/Fib.hpp src/Stats.hpp src/Sequencer.hpp src/Microphone.hpp src/RealTime.hpp src/Logger.hpp src/AudioBuffer.hpp src/FFT.hpp

OUTFILES=out/Logger.o out/RealTime.o out/Sequencer.o out/Microphone.o out/AudioBuffer.o out/FFT.o
FILES=fib stat sequencer $(OUTFILES)

all: sequencer

########################### APP BUILD ###########################
fib: src/Fib.cpp $(HFILES)
	$(CC) $(CFLAGS) -o $@ $<

stat: src/Stats.cpp $(HFILES) 
	$(CC) $(CFLAGS) -o $@ $<

sequencer: src/Main.cpp $(OUTFILES) $(HFILES) led_blink.a
	$(CC) $(CFLAGS) -o $@ $< $(OUTFILES) $(LIBS) led_blink.a

out/Logger.o: src/Logger.cpp src/Logger.hpp 
	mkdir -p out
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $<

out/FFT.o: src/FFT.cpp src/FFT.hpp out/Logger.o
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $<

out/AudioBuffer.o: src/AudioBuffer.cpp src/AudioBuffer.hpp
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $<

out/RealTime.o: src/RealTime.cpp src/RealTime.hpp out/Logger.o
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $< 

out/Microphone.o: src/Microphone.cpp src/Microphone.hpp
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $< 

out/Sequencer.o: src/Sequencer.cpp $(HFILES)
	$(CC) $(CFLAGS) -c -o $@ $<


FORCE: ;
########################### APP BUILD ###########################

########################### LIB BUILD ###########################

ASSEMBLY_CC=gcc
ASSEMBLY_CFLAGS=
ASSEMBLY=led_blink.a
ASSEMBLY_LIBS=-lm
ASSEMBLY_SRCS=$(wildcard ws2812b-test/*.c)
ASSEMBLY_OBJS=$(ASSEMBLY_SRCS:.c=.o)

# for each file in $(ASSEMBLY_SRCS), create a .o file in the same directory
led_blink.a: $(ASSEMBLY_SRCS)
	for file in $(ASSEMBLY_SRCS); do \
		echo "Compiling $$file"; \
		$(ASSEMBLY_CC) $(ASSEMBLY_CFLAGS) -c $$file -o $${file%.c}.o; \
	done
	ar rcs $@ $(ASSEMBLY_OBJS) 

########################### LIB BUILD ###########################

###########################    UTIL   ###########################

clean:
	rm -f $(FILES)
	rm -f $(ASSEMBLY)
	rm -f $(ASSEMBLY_OBJS)

###########################    UTIL   ###########################
