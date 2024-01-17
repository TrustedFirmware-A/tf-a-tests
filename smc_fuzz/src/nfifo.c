/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * FIFO for matching strings to integers
 */

#include "nfifo.h"

#ifdef SMC_FUZZ_TMALLOC
#define GENMALLOC(x)	malloc((x))
#define GENFREE(x)	free((x))
#else
#define GENMALLOC(x)	smcmalloc((x), mmod)
#define GENFREE(x)	smcfree((x), mmod)
#endif

/*
 * Initialization of FIFO
 */
void nfifoinit(struct nfifo *nf, struct memmod *mmod)
{
	nf->nent = 0;
	nf->thent = NFIFO_Q_THRESHOLD;
	nf->lnme = GENMALLOC(nf->thent * sizeof(char *));
}

/*
 * push string to FIFO for automatic numerical assignment
 */
void pushnme(char *nme, struct nfifo *nf, struct memmod *mmod)
{
	char **tnme;

	if (searchnme(nme, nf, mmod) == -1) {
		if (nf->nent >= nf->thent) {
			nf->thent += NFIFO_Q_THRESHOLD;
			tnme = GENMALLOC(nf->thent * sizeof(char *));
			for (unsigned int x = 0; x < nf->nent; x++) {
				tnme[x] = GENMALLOC(1 * sizeof(char[MAX_NAME_CHARS]));
				strlcpy(tnme[x], nf->lnme[x], MAX_NAME_CHARS);
			}
			tnme[nf->nent] = GENMALLOC(1 * sizeof(char[MAX_NAME_CHARS]));
			strlcpy(tnme[nf->nent], nme, MAX_NAME_CHARS);
			for (unsigned int x = 0; x < nf->nent; x++) {
				GENFREE(nf->lnme[x]);
			}
			GENFREE(nf->lnme);
			nf->lnme = tnme;
		} else {
			nf->lnme[nf->nent] = GENMALLOC(1 * sizeof(char[MAX_NAME_CHARS]));
			strlcpy(nf->lnme[nf->nent], nme, MAX_NAME_CHARS);
		}
		nf->nent++;
	}
}

/*
 * Find name associated with numercal designation
 */
char *readnme(int ent, struct nfifo *nf, struct memmod *mmod)
{
	return nf->lnme[ent];
}

/*
 * Search FIFO for integer given an input string returns -1
 * if not found
 */
int searchnme(char *nme, struct nfifo *nf, struct memmod *mmod)
{
	for (unsigned int x = 0; x < nf->nent; x++) {
		if (strcmp(nf->lnme[x], nme) == CMP_SUCCESS) {
			return (x + 1);
		}
	}
	return -1;
}

/*
 * Print of all elements of FIFO string and associated integer
 */
void printent(struct nfifo *nf)
{
	for (unsigned int x = 0; x < nf->nent; x++) {
		printf("nfifo entry %s has value %d\n", nf->lnme[x], x);
	}
}
