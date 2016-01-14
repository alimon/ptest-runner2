RELEASE=$(shell echo $$RELEASE)
MEMCHECK=$(shell echo $$MEMCHECK)

CC=cc
CFLAGS=-std=gnu99 -pedantic -Wall -Werror -I .
ifeq ($(RELEASE), 1)
CFLAGS+= -O2 -DRELEASE
else
CFLAGS+= -g
endif
ifeq ($(MEMCHECK), 1)
CFLAGS+= -DMEMCHECK
endif
LDFLAGS=

BASE_SOURCES=utils.c ptest_list.c
SOURCES=main.c $(BASE_SOURCES)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=ptest-runner

TEST_SOURCES=tests/main.c tests/ptest_list.c tests/utils.c $(BASE_SOURCES)
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_EXECUTABLE=ptest-runner-test
TEST_LDFLAGS=-lm -lrt -lpthread
TEST_LIBSTATIC=-lcheck

TEST_DATA=$(shell echo `pwd`/tests/data)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

tests: $(TEST_SOURCES) $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJECTS)
	$(CC) $(LDFLAGS) $(TEST_LDFLAGS) $(TEST_OBJECTS) -o $@ $(TEST_LIBSTATIC)

check: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE) -d $(TEST_DATA)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJECTS) $(TEST_EXECUTABLE) $(TEST_OBJECTS)

.PHONY: clean tests
