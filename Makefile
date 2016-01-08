CC=gcc
CFLAGS=-I . -g -Wall -Werror
LDFLAGS=

BASE_SOURCES=utils.c ptest_list.c
SOURCES=main.c $(BASE_SOURCES)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=ptest-runner

TEST_SOURCES=tests/main.c tests/ptest_list.c $(BASE_SOURCES)
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_EXECUTABLE=ptest-runner-test
TEST_LDFLAGS=-lm -lrt -lpthread
TEST_LIBSTATIC=-lcheck

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

tests: $(TEST_SOURCES) $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJECTS)
	$(CC) $(LDFLAGS) $(TEST_LDFLAGS) $(TEST_OBJECTS) -o $@ $(TEST_LIBSTATIC)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJECTS) $(TEST_EXECUTABLE) $(TEST_OBJECTS)

.PHONY: clean tests
