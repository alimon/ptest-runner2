#ifndef _PTEST_RUNNER_UTILS_H_
#define _PTEST_RUNNER_UTILS_H_

#define CHECK_ALLOCATION(p, size, exit_on_null) \
	check_allocation1(p, size, __FILE__, __LINE__, exit_on_null)

extern void check_allocation1(void *, size_t, char *, int, int);

#endif
