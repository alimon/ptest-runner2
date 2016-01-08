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
