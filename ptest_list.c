#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"
#include "ptest_list.h"

struct ptest_list *
ptest_list_alloc()
{
	struct ptest_list *p = malloc(sizeof(struct ptest_list));
	CHECK_ALLOCATION(p, sizeof(struct ptest_list), 0);
	if (p != NULL) {
		p->ptest = NULL;
		p->run_ptest = NULL;

		p->next = NULL;
		p->prev = NULL;
	}

	return p;
}

void
ptest_list_free(struct ptest_list *p)
{
	free(p->ptest);
	free(p->run_ptest);
	free(p);
}

int
ptest_list_free_all(struct ptest_list *head)
{
	int i = 0;
	struct ptest_list *p, *q;

	p = head;
	while (p != NULL) {
		q = p;
		p = p->next;

		ptest_list_free(q);
		i++;
	}

	return i;
}

int
ptest_list_length(struct ptest_list *head)
{
	int i = 0;
	struct ptest_list *p;

	for (p = head->next; p != NULL; p = p->next)
		i++;

	return i;
}

struct ptest_list *
ptest_list_search(struct ptest_list *head, char *ptest)
{
	struct ptest_list *q = NULL;
	struct ptest_list *p;

	for (p = head; p != NULL; p = p->next) {
		if (p->ptest == NULL) 
			continue;

		if (strcmp(p->ptest, ptest) == 0) {
			q = p;
			break;
		}
	}

	return q;
}

struct ptest_list *
ptest_list_add(struct ptest_list *head, char *ptest, char *run_ptest)
{
	struct ptest_list *n = ptest_list_alloc();
	struct ptest_list *p;

	n->ptest = ptest;
	n->run_ptest = run_ptest;

	n->prev = NULL;
	n->next = NULL;

	for (p = head; p->next != NULL; p = p->next);

	n->prev = p;
	p->next = n;

	return n;
}

struct ptest_list *
ptest_list_remove(struct ptest_list *head, char *ptest, int free)
{
	struct ptest_list *p = ptest_list_search(head, ptest);
	struct ptest_list *q, *r;

	if (p != NULL) {
		q = p->prev;
		r = p->next;

		q->next = r;

		if (free) {
			ptest_list_free(p);
			p = NULL;
		}
	}

	return p;
}
