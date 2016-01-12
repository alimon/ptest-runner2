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

#define _GNU_SOURCE 
#include <stdio.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include "ptest_list.h"
#include "utils.h"

void
check_allocation1(void *p, size_t size, char *file, int line, int exit_on_null)
{
	if (p == NULL) {
		fprintf(stderr, "Failed to allocate memory %ld bytes at %s,"
				" line %d.\n", size, file, line);
		if (exit_on_null)
			exit(1);
	}
}


struct ptest_list *
get_available_ptests(const char *dir)
{
	struct ptest_list *head;
	struct stat st_buf;

	int n, i;
	struct dirent **namelist;
	int fail;
	int saved_errno;

	do
	{
		head = ptest_list_alloc();
		CHECK_ALLOCATION(head, sizeof(struct ptest_list *), 0);
		if (head == NULL)
			break;

		if (stat(dir, &st_buf) == -1) {
			PTEST_LIST_FREE_CLEAN(head);
			break;
		}

		if (!S_ISDIR(st_buf.st_mode)) {
			PTEST_LIST_FREE_CLEAN(head);
			errno = EINVAL;
			break;
		}

		n = scandir(dir, &namelist, NULL, alphasort);
		if (n == -1) {
			PTEST_LIST_FREE_CLEAN(head);
			break;
		}


		fail = 0;
		for (i = 0; i < n; i++) {
			char *run_ptest;

			char *d_name = strdup(namelist[i]->d_name);
			CHECK_ALLOCATION(d_name, sizeof(namelist[i]->d_name), 0);
			if (d_name == NULL) {
				fail = 1;
				saved_errno = errno;
				break;
			}

			if (strcmp(d_name, ".") == 0 ||
			    strcmp(d_name, "..") == 0) {
				free(d_name);
				continue;
			}

			if (asprintf(&run_ptest, "%s/%s/ptest/run-ptest",
			    dir, d_name) == -1)  {
				fail = 1;
				saved_errno = errno;
				free(d_name);
				break;
			}

			if (stat(run_ptest, &st_buf) == -1) {
				free(run_ptest);
				free(d_name);
				continue;
			}

			if (!S_ISREG(st_buf.st_mode)) {
				free(run_ptest);
				free(d_name);
				continue;
			}

			struct ptest_list *p = ptest_list_add(head,
				d_name, run_ptest);
			CHECK_ALLOCATION(p, sizeof(struct ptest_list *), 0);
			if (p == NULL) {
				fail = 1;
				saved_errno = errno;
				free(run_ptest);
				free(d_name);
				break;
			}
		}

		for (i = 0 ; i < n; i++)
			free(namelist[i]);
		free(namelist);

		if (fail) {
			PTEST_LIST_FREE_ALL_CLEAN(head);
			errno = saved_errno;
			break;
		}
	} while (0);

	return head;
}

int
print_ptests(struct ptest_list *head, FILE *fp)
{
	if (ptest_list_length(head) <= 0) {
		fprintf(fp, PRINT_PTESTS_NOT_FOUND);
		return 1;
	} else {
		struct ptest_list *n;
		fprintf(fp, PRINT_PTESTS_AVAILABLE);
		PTEST_LIST_ITERATE_START(head, n);
			fprintf(fp, "%s\t%s\n", n->ptest, n->run_ptest);
		PTEST_LIST_ITERATE_END;
		return 0;
	}
}
