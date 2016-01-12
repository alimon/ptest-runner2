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

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <check.h>

#include "ptest_list.h"
#include "utils.h"

#define PRINT_PTEST_BUF_SIZE 8192
#define PRINT_PTEST_MAX_LINE 512

extern char *opts_directory;

static char *ptests_found[] = {
	"bash",
	"gcc",
	"glibc",
	"python",
	NULL
};
static int ptests_found_length = 4;
static char *ptests_not_found[] = {
	"busybox",
	"perl",
	NULL,
};

START_TEST(test_get_available_ptests)
{
	struct ptest_list *head = get_available_ptests(opts_directory);
	int i;

	ck_assert(ptest_list_length(head) == ptests_found_length);

	for (i = 0; ptests_found[i] != NULL; i++)
		ck_assert(ptest_list_search(head, ptests_found[i]) != NULL);
	for (i = 0; ptests_not_found[i] != NULL; i++)
		ck_assert(ptest_list_search(head, ptests_not_found[i]) == NULL);

	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_print_ptests)
	struct ptest_list *head;

	char *buf;
	size_t size = PRINT_PTEST_BUF_SIZE;
	FILE *fp;

	char *line;
	char line_buf[PRINT_PTEST_BUF_SIZE];

	int i;
	char *ptest;

	fp = open_memstream(&buf, &size);
	ck_assert(fp != NULL);

	ck_assert(print_ptests(NULL, fp) == 1);
	line = fgets(line_buf, PRINT_PTEST_BUF_SIZE, fp);
	ck_assert(line != NULL);
	ck_assert(strcmp(line, PRINT_PTESTS_NOT_FOUND) == 0);

	head = ptest_list_alloc();
	ck_assert(print_ptests(head, fp) == 1);
	ptest_list_free_all(head);
	line = fgets(line_buf, PRINT_PTEST_BUF_SIZE, fp);
	ck_assert(line != NULL);
	ck_assert(strcmp(line, PRINT_PTESTS_NOT_FOUND) == 0);

	head = get_available_ptests(opts_directory);
	ck_assert(print_ptests(head, fp) == 0);
	ptest_list_free_all(head);
	line = fgets(line_buf, PRINT_PTEST_BUF_SIZE, fp);
	ck_assert(line != NULL);
	ck_assert(strcmp(line, PRINT_PTESTS_AVAILABLE) == 0);
	i = 0;
	while ((ptest = ptests_found[i]) != NULL) {
		line = fgets(line_buf, PRINT_PTEST_BUF_SIZE, fp);
		ck_assert(line != NULL);
		line[strlen(ptest)] = '\0'; // XXX: Only compare the name part
		ck_assert(strcmp(line, ptest) == 0);
		i++;
	}

	fclose(fp);
	free(buf);
END_TEST

START_TEST(test_filter_ptests)
	struct ptest_list *head = get_available_ptests(opts_directory);
	struct ptest_list *head_new;
	char *ptest_not_exists[] = {
		"glib",
	};
	char *ptest_exists[] = {
		"glibc",
		"gcc",
		"python",
	};

	ck_assert(filter_ptests(NULL, NULL, -1) == NULL && errno == EINVAL);
	ck_assert(filter_ptests(head, ptest_not_exists, 1) == NULL);

	head_new = filter_ptests(head, ptest_exists, 3);
	ck_assert(head_new != NULL);
	ck_assert(ptest_list_length(head_new) == 3);

	ptest_list_free_all(head);
	ptest_list_free_all(head_new);
END_TEST

Suite *
utils_suite()
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("utils");
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_get_available_ptests);
	tcase_add_test(tc_core, test_print_ptests);
	tcase_add_test(tc_core, test_filter_ptests);

	suite_add_tcase(s, tc_core);

	return s;
}
