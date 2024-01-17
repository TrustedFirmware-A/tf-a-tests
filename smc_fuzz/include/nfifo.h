/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef NFIFO_H
#define NFIFO_H

#define CMP_SUCCESS 0
#define NFIFO_Q_THRESHOLD 10

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smcmalloc.h"

struct nfifo {
	char **lnme;
	int nent;
	int thent;
};

void nfifoinit(struct nfifo *nf, struct memmod *mmod);
void pushnme(char *nme, struct nfifo *nf, struct memmod *mmod);
char *readnme(int ent, struct nfifo *nf, struct memmod *mmod);
int searchnme(char *nme, struct nfifo *nf, struct memmod *mmod);
void printent(struct nfifo *nf);

#endif /* NFIFO_H */
