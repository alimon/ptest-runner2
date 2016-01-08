#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <check.h>

#include "ptest_list.h"

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
	struct ptest_list *head = ptest_list_alloc();
	int n = 5;
	int i;
 
	for (i = 0; i < n; i++)
		ptest_list_add(head, NULL, NULL);

	ptest_list_free_all(head);
}
END_TEST


START_TEST(test_length)
{
	struct ptest_list *head = ptest_list_alloc();
	int n = 5;
	int i;
 
	for (i = 0; i < n; i++)
		ptest_list_add(head, NULL, NULL);

	ck_assert_int_eq(ptest_list_length(head), n);
	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_search)
{
	struct ptest_list *head = ptest_list_alloc();
	int i;
	int n = 6;
	char *ptest_names[] = {
		"python",
		"glibc",
		"busybox",
		"gcc",
		"gdb",
		"perl",
	};
	char *ptest;

	for (i = 0; i < n; i++) {
		ptest = strdup(ptest_names[i]);
		ptest_list_add(head, ptest, NULL);
	}

	for (i = n - 1; i >= 0; i--)
		ck_assert(ptest_list_search(head, ptest_names[i]) != NULL);

	ptest_list_free_all(head);
}
END_TEST

START_TEST(test_remove)
{
	struct ptest_list *head = ptest_list_alloc();
	int i;
	int n = 6;
	char *ptest_names[] = {
		"python",
		"glibc",
		"busybox",
		"gcc",
		"gdb",
		"perl",
	};
	char *ptest;

	for (i = 0; i < n; i++) {
		ptest = strdup(ptest_names[i]);
		ptest_list_add(head, ptest, NULL);
	}

	/* Remove node free'ing */
	ck_assert(ptest_list_remove(head, "python", 1) == NULL);
	ck_assert(ptest_list_search(head, "python") == NULL);
	n = n -1;
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

	suite_add_tcase(s, tc_core);

	return s;
}
