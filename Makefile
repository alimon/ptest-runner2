CC=gcc
CFLAGS=-g -Wall -Werror
LDFLAGS=

SOURCES=main.c utils.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=ptest-runner

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJECTS)
