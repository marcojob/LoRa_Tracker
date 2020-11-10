CC = g++

run:
	./main

main: run
	$(CC) -O3 main.cpp -o $@

clean:
	rm -rf *.o *.s *.d main

all: main
.DEFAULT_GOAL := all