#ifndef _PTEST_RUNNER_LIST_H_
#define _PTEST_RUNNER_LIST_H_

struct ptest_list {
	char *ptest;
	char *run_ptest;

	struct ptest_list *next;
	struct ptest_list *prev;
};

extern struct ptest_list *ptest_list_alloc();
extern void ptest_list_free(struct ptest_list *);
extern int ptest_list_free_all(struct ptest_list *);

extern int ptest_list_length(struct ptest_list *);
extern struct ptest_list *ptest_list_search(struct ptest_list *, char *);
extern struct ptest_list *ptest_list_add(struct ptest_list *, char *, char *);
extern struct ptest_list *ptest_list_remove(struct ptest_list *, char *, int);

#endif // _PTEST_RUNNER_LIST_H_
