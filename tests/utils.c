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
#include <stdlib.h>
#include <stdio.h>
#include <check.h>

#include "ptest_list.h"
#include "utils.h"

extern char *opts_directory;

START_TEST(test_get_available_ptests)
{
	struct ptest_list *head = get_available_ptests(opts_directory);
	int expected_length = 4;
	char *ptests[] = {
		"gcc",
		"glibc",
		"python",
		"bash",
		NULL,
	};
	char *ptests_not_found[] = {
		"busybox",
		"perl",
		NULL,
	};
	int i;

	ck_assert(ptest_list_length(head) == expected_length);

	for (i = 0; ptests[i] != NULL; i++)
		ck_assert(ptest_list_search(head, ptests[i]) != NULL);
	for (i = 0; ptests_not_found[i] != NULL; i++)
		ck_assert(ptest_list_search(head, ptests_not_found[i]) == NULL);

	ptest_list_free_all(head);
}
END_TEST

Suite *
utils_suite()
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("utils");
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_get_available_ptests);

	suite_add_tcase(s, tc_core);

	return s;
}
