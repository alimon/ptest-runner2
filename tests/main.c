/**
* Copyright (c) 2016 Intel Corporation
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
* AUTHORS
* 	Aníbal Limón <anibal.limon@intel.com>
*/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

typedef Suite *(SuiteFunction)(void);

extern Suite *ptest_list_suite(void);
extern Suite *utils_suite(void);
static SuiteFunction *suites[] = {
	&ptest_list_suite,
	&utils_suite,
	NULL,
};

static inline void
print_usage(FILE *stream, char *progname)
{
	fprintf(stream, "Usage: %s <-d directory>\n", progname);
}

char *opts_directory;

int
main(int argc, char *argv[])
{
	int opt;
	int i;
	int number_failed;
	SuiteFunction *sf;

	opts_directory = NULL;

	while ((opt = getopt(argc, argv, "d:t:h")) != -1) {
		switch (opt) {
			case 'd':
				opts_directory = strdup(optarg);
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

	if (opts_directory == NULL) {
		print_usage(stdout, argv[0]);
		exit(1);
	}

	i = 0;
	number_failed = 0;
	sf = suites[i];
	while (sf != NULL) {
		Suite *s = (*sf)();
		SRunner *sr = srunner_create(s);

		srunner_run_all(sr, CK_VERBOSE);
		number_failed += srunner_ntests_failed(sr);
		srunner_free(sr);

		i++;
		sf = suites[i];
	}

	return number_failed;
}
