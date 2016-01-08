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

#include <stdlib.h>
#include <stdio.h>

#include <check.h>

typedef Suite *(SuiteFunction)(void);

extern Suite *ptest_list_suite(void);
static SuiteFunction *suites[] = {
	&ptest_list_suite,
	NULL,
};

int
main(int argc, char *argv[])
{
	int i = 0;
	int number_failed = 0;
	SuiteFunction *sf;

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
