CC=gcc
CFLAGS=-Wall -c
LD=gcc
LDFLAGS=

all: brnflip

brnflip: brnflip.o cli.o
	$(LD) $(LDLAGS) -o brnflip brnflip.o cli.o

*.o: *.c
	$(CC) $(CFLAGS) -c *.c

clean:
	rm brnflip
	rm *.o

