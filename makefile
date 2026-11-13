# definizione del compilatore e dei flag di compilazione
CC=gcc
CFLAGS=-std=c11 -Wall -g -O3 -pthread 
LDLIBS=-lm -pthread

EXECS= msf.out

all: $(EXECS) 

# regola per la creazione degli eseguibili includendo i nuovi file oggetto
msf.out: msf.o grafo_helper.o
	$(CC) -o $@ $^ $(LDLIBS)

# Regola esplicita per msf.o
msf.o: msf.c grafo_helper.h
	$(CC) $(CFLAGS) -c msf.c

# Regola per il file helper
grafo_helper.o: grafo_helper.c grafo_helper.h
	$(CC) $(CFLAGS) -c grafo_helper.c

# regola per la creazione di file oggetto generici
%.o: %.c
	$(CC) $(CFLAGS) -c $<
 
clean: 
	rm -f *.o $(EXECS)
