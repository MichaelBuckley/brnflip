CC=gcc
CFLAGS=-Wall -c
LD=gcc
LDFLAGS=

all: brnflip

brnflip: brnflip.o cli.o
	$(LD) $(LDLAGS) -o brnflip brnflip.o cli.o

*.o: *.c
	$(CC) ($CCFLAGS) -o *.o *.c

clean:
	rm brnflip
	rm *.o

