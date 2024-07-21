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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <errno.h>

#include "ptest_list.h"

extern Suite *ptest_list_suite(void);

static int ptests_num = 6;
static char *ptest_names[] = {
	"python",
	"glibc",
	"busybox",
	"gcc",
	"gdb",
	"perl",
};

START_TEST(test_alloc_free)
{
	struct ptest_list *p = ptest_list_alloc();
	ck_assert(p != NULL);
	ptest_list_free(p);
}
END_TEST

START_TEST(test_add)
{
	struct ptest_list *head = ptest_list_alloc();
	ck_assert(ptest_list_add(head, strdup("perl"), NULL) != NULL);
	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_free_all)
{
	struct ptest_list *head = NULL;
	int i;
 
	ck_assert(ptest_list_free_all(head) == -1);
	ck_assert(errno == EINVAL);

	head = ptest_list_alloc();
	for (i = 0; i < ptests_num; i++)
		ptest_list_add(head, strdup(ptest_names[i]), NULL);

	ptest_list_free_all(head);
}
END_TEST


START_TEST(test_length)
{
	struct ptest_list *head = NULL;
	int i;

	ck_assert(ptest_list_length(head) == -1);
	ck_assert(errno == EINVAL);
 
	head = ptest_list_alloc();
	for (i = 0; i < ptests_num; i++)
		ptest_list_add(head, strdup(ptest_names[i]), NULL);

	ck_assert_int_eq(ptest_list_length(head), ptests_num);
	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_search)
{
	struct ptest_list *head = NULL;
	int i;
	char *ptest;

	ck_assert(ptest_list_search(head, NULL) == NULL);
	ck_assert(errno == EINVAL);

	head = ptest_list_alloc();
	for (i = 0; i < ptests_num; i++) {
		ptest = strdup(ptest_names[i]);
		ptest_list_add(head, ptest, NULL);
	}

	for (i = ptests_num - 1; i >= 0; i--)
		ck_assert(ptest_list_search(head, ptest_names[i]) != NULL);

	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_remove)
{
	struct ptest_list *head = ptest_list_alloc();
	int i;
	char *ptest;
	int n = ptests_num;

	for (i = 0; i < ptests_num; i++) {
		ptest = strdup(ptest_names[i]);
		ptest_list_add(head, ptest, NULL);
	}

	/* Remove node free'ing */
	ck_assert(ptest_list_remove(head, "python", 1) == NULL);
	ck_assert(ptest_list_search(head, "python") == NULL);
	n = n - 1;
	ck_assert_int_eq(ptest_list_length(head), n);

	/* Remove node without free'ing */
	ck_assert(ptest_list_remove(head, "python", 1) == NULL);
	struct ptest_list *p = ptest_list_remove(head, "perl", 0);
	ck_assert(p != NULL);
	ptest_list_free(p);
	ck_assert(ptest_list_search(head, "perl") == NULL);
	n = n - 1;
	ck_assert_int_eq(ptest_list_length(head), n);

	ck_assert(ptest_list_search(head, "busybox") != NULL);
	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_remove_first)
{
	struct ptest_list *head = ptest_list_alloc();
	struct ptest_list *p;

	ptest_list_add(head, strdup("a"), NULL);
	ptest_list_add(head, strdup("b"), NULL);
	ptest_list_add(head, strdup("c"), NULL);

	p = ptest_list_remove(head, "a", 0);
	ck_assert_ptr_nonnull(p);
	ck_assert_str_eq(p->ptest, "a");
	ptest_list_free(p);

	ck_assert_ptr_null(ptest_list_search(head, "a"));
	ck_assert_ptr_nonnull(ptest_list_search(head, "b"));
	ck_assert_ptr_nonnull(ptest_list_search(head, "c"));
	ck_assert_int_eq(ptest_list_length(head), 2);

	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_remove_last)
{
	struct ptest_list *head = ptest_list_alloc();
	struct ptest_list *p;

	ptest_list_add(head, strdup("a"), NULL);
	ptest_list_add(head, strdup("b"), NULL);
	ptest_list_add(head, strdup("c"), NULL);

	p = ptest_list_remove(head, "c", 0);
	ck_assert_ptr_nonnull(p);
	ck_assert_str_eq(p->ptest, "c");
	ptest_list_free(p);

	ck_assert_ptr_nonnull(ptest_list_search(head, "a"));
	ck_assert_ptr_nonnull(ptest_list_search(head, "b"));
	ck_assert_ptr_null(ptest_list_search(head, "c"));
	ck_assert_int_eq(ptest_list_length(head), 2);

	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_remove_all)
{
	struct ptest_list *head = ptest_list_alloc();

	ptest_list_add(head, strdup("a"), NULL);
	ptest_list_add(head, strdup("b"), NULL);

	ck_assert_ptr_null(ptest_list_remove(head, "a", 1));
	ck_assert_ptr_null(ptest_list_remove(head, "b", 1));
	ck_assert_int_eq(ptest_list_length(head), 0);

	ptest_list_free_all(head);
}
END_TEST

Suite *
ptest_list_suite()
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("ptest_list");
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_alloc_free);
	tcase_add_test(tc_core, test_add);
	tcase_add_test(tc_core, test_free_all);
	tcase_add_test(tc_core, test_length);
	tcase_add_test(tc_core, test_search);
	tcase_add_test(tc_core, test_remove);
	tcase_add_test(tc_core, test_remove_first);
	tcase_add_test(tc_core, test_remove_last);
	tcase_add_test(tc_core, test_remove_all);

	suite_add_tcase(s, tc_core);

	return s;
}
