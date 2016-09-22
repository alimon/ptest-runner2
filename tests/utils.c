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
	"fail",
	"gcc",
	"glibc",
	"hang",
	"python",
	NULL
};
static int ptests_found_length = 6;
static char *ptests_not_found[] = {
	"busybox",
	"perl",
	NULL,
};

static inline void
find_word(int *found, const char *line, const char *word)
{

	char *pivot = NULL;

	pivot = strdup(line);
	pivot[strlen(word)] = '\0';
	if (strcmp(pivot, word) == 0) { *found = 1; }
	free(pivot);
}

static void test_ptest_expected_failure(struct ptest_list *, const int, char *,
	void (*h_analizer)(const int, FILE *, FILE *));

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

START_TEST(test_run_ptests)
	struct ptest_list *head; 
	int timeout = 1;
	int rc;

	char *buf_stdout;
	size_t size_stdout = PRINT_PTEST_BUF_SIZE;
	FILE *fp_stdout;
	char *buf_stderr;
	size_t size_stderr = PRINT_PTEST_BUF_SIZE;
	FILE *fp_stderr;

	fp_stdout = open_memstream(&buf_stdout, &size_stdout);
	ck_assert(fp_stdout != NULL);
	fp_stderr = open_memstream(&buf_stderr, &size_stderr);
	ck_assert(fp_stderr != NULL);

	head = get_available_ptests(opts_directory);
	ptest_list_remove(head, "hang", 1);
	ptest_list_remove(head, "fail", 1);

	rc = run_ptests(head, timeout, "test_run_ptests", fp_stdout, fp_stderr);
	ck_assert(rc == 0);
	ptest_list_free_all(head);

	fclose(fp_stdout);
	free(buf_stdout);
	fclose(fp_stderr);
	free(buf_stderr);
END_TEST

static void
search_for_timeout(const int rp, FILE *fp_stdout, FILE *fp_stderr)
{
        const char *ptest_hang = "hang";
        const char *timeout_str = "TIMEOUT";
        char line_buf[PRINT_PTEST_BUF_SIZE];
	int found_hang = 0;
	int found_timeout = 0;
	int next = 0;
	char *line = NULL;

	ck_assert(rp != 0);

	while ((line = fgets(line_buf, PRINT_PTEST_BUF_SIZE, fp_stdout)) != NULL) {
		if (next) {
                        find_word(&found_timeout, line, timeout_str);
                } else {
                        find_word(&next, line, ptest_hang); // XXX: Only compare the name part
                }
	}

	ck_assert(found_timeout == 1);

        /* Test stderr output */
        line = fgets(line_buf, PRINT_PTEST_BUF_SIZE, fp_stderr);
        find_word(&found_hang, line, ptest_hang);

	ck_assert(found_hang == 1);
}

START_TEST(test_run_timeout_ptest)
	struct ptest_list *head = get_available_ptests(opts_directory);
	int timeout = 1;

	test_ptest_expected_failure(head, timeout, "hang", search_for_timeout);

	ptest_list_free_all(head);
END_TEST

static void
search_for_fail(const int rp, FILE *fp_stdout, FILE *fp_stderr)
{
        const char *fail_str = "ERROR: Exit status is";
        char line_buf[PRINT_PTEST_BUF_SIZE];
        int found_fail = 0;
        char *line = NULL;

        ck_assert(rp != 0);

        while ((line = fgets(line_buf, PRINT_PTEST_BUF_SIZE, fp_stdout)) != NULL) {
		find_word(&found_fail, line, fail_str);
        }

        ck_assert(found_fail == 1);
}

START_TEST(test_run_fail_ptest)
	struct ptest_list *head = get_available_ptests(opts_directory);
	int timeout = 1;

	test_ptest_expected_failure(head, timeout, "fail", search_for_fail);

	ptest_list_free_all(head);
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
	tcase_add_test(tc_core, test_run_ptests);
	tcase_add_test(tc_core, test_run_timeout_ptest);
	tcase_add_test(tc_core, test_run_fail_ptest);

	suite_add_tcase(s, tc_core);

	return s;
}

static void
test_ptest_expected_failure(struct ptest_list *head, const int timeout, char *progname,
		void (*h_analizer)(const int, FILE *, FILE *))
{
	char *buf_stdout;
	size_t size_stdout = PRINT_PTEST_BUF_SIZE;
	FILE *fp_stdout;
	char *buf_stderr;
	size_t size_stderr = PRINT_PTEST_BUF_SIZE;
	FILE *fp_stderr;

	fp_stdout = open_memstream(&buf_stdout, &size_stdout);
	ck_assert(fp_stdout != NULL);
	fp_stderr = open_memstream(&buf_stderr, &size_stderr);
	ck_assert(fp_stderr != NULL);

	{
		struct ptest_list *filtered = filter_ptests(head, &progname, 1);
		ck_assert(ptest_list_length(filtered) == 1);

		h_analizer(
			run_ptests(filtered, timeout, progname, fp_stdout, fp_stderr),
			fp_stdout, fp_stderr
		);

		PTEST_LIST_FREE_ALL_CLEAN(filtered);
	}

	fclose(fp_stdout);
	free(buf_stdout);
	fclose(fp_stderr);
	free(buf_stderr);
}
