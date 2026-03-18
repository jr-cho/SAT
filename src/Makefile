CC=gcc
CFLAGS=-Wall

all: orchestrator

orchestrator: main.o runner.o
	$(CC) $(CFLAGS) -o orchestrator main.o runner.o

clean:
	rm -f *.o orchestrator
