#include <stdlib.h>
#include <stdio.h>

extern void
check_allocation1(void *p, size_t size, char *file, int line, int exit_on_null)
{
	if (p == NULL) {
		fprintf(stderr, "Failed to allocate memory %ld bytes at %s,"
				" line %d.\n", size, file, line);
		if (exit_on_null)
			exit(1);
	}
}
