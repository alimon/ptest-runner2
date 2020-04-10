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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * AUTHORS
 * 	Aníbal Limón <anibal.limon@intel.com>
 */

#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef MEMCHECK
#ifdef RELEASE
#error "You can't use MEMCHECK when RELEASE is active."
#endif

#include <mcheck.h>
#endif

#include "utils.h"

#ifndef DEFAULT_DIRECTORY
#define DEFAULT_DIRECTORY "/usr/lib"
#endif
#define DEFAULT_TIMEOUT 300

static inline void
print_usage(FILE *stream, char *progname)
{
	fprintf(stream, "Usage: %s [-d directory directory2 ...] [-e exclude] [-l list] [-t timeout]"
			" [-x xml-filename] [-h] [ptest1 ptest2 ...]\n", progname);
}

static char **
str2array(char *str, const char *delim, int *num)
{
	char *c, *tok;
	char **array;
	int i;

	c = str;
	*num = 1;
	while (*c) {
		if (isspace(*c))
			(*num)++;
		c++;
	}


	array = malloc((size_t)*num * sizeof(char**));
	CHECK_ALLOCATION(array, 1, 1);

	i = 0;
	tok = strtok_r(str, delim, &c);
	array[i] = strdup(tok);
	CHECK_ALLOCATION(array[i], 1, 1);
	i++;
	while ((tok = strtok_r(NULL, " ", &c)) != NULL) {
		array[i] = strdup(tok);
		CHECK_ALLOCATION(array[i], 1, 1);
		i++;
	}

	return array;
}

int
main(int argc, char *argv[])
{
	int opt;
	int ptest_num = 0;
	int i;
	int rc;
	int ptest_exclude_num = 0;

#ifdef MEMCHECK
	mtrace();
#endif

	struct ptest_list *head, *run;
	struct ptest_options opts;

	opts.dirs = malloc(sizeof(char **) * 1);
	CHECK_ALLOCATION(opts.dirs, 1, 1);
	opts.dirs[0] = strdup(DEFAULT_DIRECTORY);
	CHECK_ALLOCATION(opts.dirs[0], 1, 1);
	opts.dirs_no = 1;
	opts.exclude = NULL;
	opts.list = 0;
	opts.timeout = DEFAULT_TIMEOUT;
	opts.ptests = NULL;
	opts.xml_filename = NULL;

	while ((opt = getopt(argc, argv, "d:e:lt:x:h")) != -1) {
		switch (opt) {
			case 'd':
				free(opts.dirs[0]);
				free(opts.dirs);
				opts.dirs = str2array(optarg, " ", &(opts.dirs_no)); 
			break;
			case 'e':
				opts.exclude = str2array(optarg, " ", &ptest_exclude_num);
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
			/* break; not needed, not reachable after exit() */
			case 'x':
				free(opts.xml_filename);
				opts.xml_filename = strdup(optarg);
				CHECK_ALLOCATION(opts.xml_filename, 1, 1);
			break;
			default:
				print_usage(stdout, argv[0]);
				exit(1);
		}
	}

	ptest_num = argc - optind;
	if (ptest_num > 0) {
		size_t size = sizeof(char *) * (unsigned int) ptest_num;
		opts.ptests = calloc(1, size);
		CHECK_ALLOCATION(opts.ptests, size, 1);

		for (i = 0; i < ptest_num; i++) {
			opts.ptests[i] = strdup(argv[argc - ptest_num + i]);
			CHECK_ALLOCATION(opts.ptests[i], 1, 1);
		}
	}

	head = NULL;
	for (i = 0; i < opts.dirs_no; i ++) {
		struct ptest_list *tmp;

		tmp = get_available_ptests(opts.dirs[i]);
		if (tmp == NULL) {
			fprintf(stderr, PRINT_PTESTS_NOT_FOUND_DIR, opts.dirs[i]);
			continue;
		}


		if (head == NULL)
			head = tmp;
		else
			head = ptest_list_extend(head, tmp);
	}
	if (head == NULL || ptest_list_length(head) == 0) {
		fprintf(stderr, PRINT_PTESTS_NOT_FOUND);
			return 1;
	}

	if (opts.list) {
		print_ptests(head, stdout);
		return 0;
	}

	run = head;
	if (ptest_num > 0) {
		for (i = 0; i < ptest_num; i++) {
			if (ptest_list_search(head, opts.ptests[i]) == NULL) {
				fprintf(stderr, "%s ptest isn't available.\n",
					opts.ptests[i]);
				return 1;
			}
		}

		run = filter_ptests(head, opts.ptests, ptest_num);
		CHECK_ALLOCATION(run, (size_t) ptest_num, 1);
		ptest_list_free_all(head);
	}

	for (i = 0; i < ptest_exclude_num; i++)
		ptest_list_remove(run, opts.exclude[i], 1);

	rc = run_ptests(run, opts, argv[0], stdout, stderr);

	ptest_list_free_all(run);

	return rc;
}
