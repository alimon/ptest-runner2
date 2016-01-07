#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

#define DEFAULT_DIRECTORY "/usr/lib"
#define DEFAULT_TIMEOUT 300

static inline void
print_usage(FILE *stream, char *progname)
{
	fprintf(stream, "Usage: %s [-d directory] [-l list] [-t timeout] "
			"[-h] [ptest1 ptest2 ...]\n", progname);
}

static struct {
	char *directory;
	int list;
	int timeout;
	char **ptests;
} opts;

int
main(int argc, char *argv[])
{
	int opt;
	int ptest_num = 0;
	int i;

	opts.directory = strdup(DEFAULT_DIRECTORY);
	opts.list = 0;
	opts.timeout = DEFAULT_TIMEOUT;
	opts.ptests = NULL;

	while ((opt = getopt(argc, argv, "d:lt:h")) != -1) {
		switch (opt) {
			case 'd':
				free(opts.directory);
				opts.directory = strdup(optarg);
				CHECK_ALLOCATION(opts.directory, 1, 1);
			break;
			case 'l':
				opts.list = 1;
			break;
			case 't':
				opts.timeout = atoi(optarg);
			break;
			case 'h':
				print_usage(stdout, argv[0]);
				exit(0);
			break;
			default:
				print_usage(stdout, argv[0]);
				exit(1);
			break;
		}
	}

	ptest_num = argc - optind;
	if (ptest_num > 0) {
		size_t size = ptest_num * sizeof(char *);
		opts.ptests = calloc(1, size);
		CHECK_ALLOCATION(opts.ptests, size, 1);

		for (i = 0; i < ptest_num; i++) {
			opts.ptests[i] = strdup(argv[argc - ptest_num + i]);
			CHECK_ALLOCATION(opts.ptests[i], 1, 1);
		}
	}

	return 0;
}
