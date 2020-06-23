CC=cc
CFLAGS=-I/usr/local/include -Wall -g -pthread
LDFLAGS=-L/usr/local/lib -lrlottie -lpng -lm

PROG=tgs2png
SOURCES=tgs2png.c

$(PROG): $(SOURCES)
	$(CC) $(CFLAGS) -o $(PROG) $(SOURCES) $(LDFLAGS)

clean:
	@rm -vf $(PROG)

.PHONY: clean
