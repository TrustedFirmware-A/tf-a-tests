/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <drivers/arm/private_timer.h>
#include <events.h>
#include "fifo3d.h"
#include <libfdt.h>

#include <power_management.h>
#include <sdei.h>
#include <tftf_lib.h>
#include <timer.h>

#include <plat_topology.h>
#include <platform.h>

#ifdef SMC_FUZZ_TMALLOC
#define GENMALLOC(x)	malloc((x))
#define GENFREE(x)	free((x))
#else
#define GENMALLOC(x)	smcmalloc((x), mmod)
#define GENFREE(x)	smcfree((x), mmod)
#endif

/*
 * Push function name string into the data structure from device tree file
 */
void push_3dfifo_fname(struct fifo3d *f3d, char *fname)
{
	strlcpy(f3d->fnamefifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1],
		fname, MAX_NAME_CHARS);
}

/*
 * Push bias value into data structure from device tree file
 */
void push_3dfifo_bias(struct fifo3d *f3d, int bias)
{
	f3d->biasfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = bias;
}

/*
 * Push function id value into data structure from device tree file
 */
void push_3dfifo_fid(struct fifo3d *f3d, int id)
{
	f3d->fidfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = id;
}


/*
 * Create new column and/or row for raw data structure for newly
 * found node from device tree.  The fifo has four elements that reflect
 * values obtained from the device tree for each node read.  This preserves
 * the hierarchy found in that file so it can be utilized in construction of
 * the smc nodes structure for final use in randomly calling the SMC functions.
 * This is essentially a bias tree in final form.
 */
void push_3dfifo_col(struct fifo3d *f3d, char *entry, struct memmod *mmod)
{

/*
 * four elements required:
 * 1. node name as a string
 * 2. function name as a string
 * 3. bias value as an integer
 * 4. id value as an integer
 */
	char ***tnnfifo;
	char ***tfnamefifo;
	int **tbiasfifo;
	int **tfidfifo;

	if (f3d->col == f3d->curr_col) {
		f3d->col++;
		f3d->curr_col++;
		int *trow;
		trow = GENMALLOC(f3d->col * sizeof(int));

		/*
		 * return if error found
		 */
		if (mmod->memerror != 0) {
			return;
		}

		for (unsigned int i = 0U; (int)i < f3d->col - 1; i++) {
			trow[i] = f3d->row[i];
		}
		if (f3d->col > 1) {
			GENFREE(f3d->row);
		}
		f3d->row = trow;
		f3d->row[f3d->col - 1] = 1;

		/*
		 * Start node creation for reading of device tree file
		 */
		tnnfifo = GENMALLOC(f3d->col * sizeof(char **));
		tfnamefifo = GENMALLOC(f3d->col * sizeof(char **));
		tbiasfifo = GENMALLOC((f3d->col) * sizeof(int *));
		tfidfifo = GENMALLOC((f3d->col) * sizeof(int *));
		for (unsigned int i = 0U; (int)i < f3d->col; i++) {
			tnnfifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *));
			tfnamefifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *));
			tbiasfifo[i] = GENMALLOC((f3d->row[i]) * sizeof(int));
			tfidfifo[i] = GENMALLOC((f3d->row[i]) * sizeof(int));
			for (unsigned int j = 0U; (int)j < f3d->row[i]; j++) {
				tnnfifo[i][j] = GENMALLOC(1 * sizeof(char[MAX_NAME_CHARS]));
				tfnamefifo[i][j] =
					GENMALLOC(1 * sizeof(char[MAX_NAME_CHARS]));
				if (!((j == f3d->row[f3d->col - 1] - 1) &&
				      (i == (f3d->col - 1)))) {
					strlcpy(tnnfifo[i][j], f3d->nnfifo[i][j], MAX_NAME_CHARS);
					strlcpy(tfnamefifo[i][j],
						f3d->fnamefifo[i][j], MAX_NAME_CHARS);
					tbiasfifo[i][j] = f3d->biasfifo[i][j];
					tfidfifo[i][j] = f3d->fidfifo[i][j];
				}
			}
		}

		/*
		 * Copy data from old raw data to new memory location
		 */
		strlcpy(tnnfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1], entry,
			MAX_NAME_CHARS);
		strlcpy(tfnamefifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1],
			"none", MAX_NAME_CHARS);
		tbiasfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = 0;
		tfidfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = 0;

		/*
		 * Free the old raw data structres
		 */
		for (unsigned int i = 0U; (int)i < f3d->col - 1; i++) {
			for (unsigned int j = 0U; (int)j < f3d->row[i]; j++) {
				GENFREE(f3d->nnfifo[i][j]);
				GENFREE(f3d->fnamefifo[i][j]);
			}
			GENFREE(f3d->nnfifo[i]);
			GENFREE(f3d->fnamefifo[i]);
			GENFREE(f3d->biasfifo[i]);
			GENFREE(f3d->fidfifo[i]);
		}
		if (f3d->col > 1) {
			GENFREE(f3d->nnfifo);
			GENFREE(f3d->fnamefifo);
			GENFREE(f3d->biasfifo);
			GENFREE(f3d->fidfifo);
		}

		/*
		 * Point to new data
		 */
		f3d->nnfifo = tnnfifo;
		f3d->fnamefifo = tfnamefifo;
		f3d->biasfifo = tbiasfifo;
		f3d->fidfifo = tfidfifo;
	}
	if (f3d->col != f3d->curr_col) {
		/*
		 *  Adding new node to raw data
		 */
		f3d->col++;
		f3d->row[f3d->col - 1]++;

		/*
		 * Create new node form device tree file
		 */
		tnnfifo = GENMALLOC(f3d->col * sizeof(char **));
		tfnamefifo = GENMALLOC(f3d->col * sizeof(char **));
		tbiasfifo = GENMALLOC((f3d->col) * sizeof(int *));
		tfidfifo = GENMALLOC((f3d->col) * sizeof(int *));
		for (unsigned int i = 0U; (int)i < f3d->col; i++) {
			tnnfifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *));
			tfnamefifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *));
			tbiasfifo[i] = GENMALLOC((f3d->row[i]) * sizeof(int));
			tfidfifo[i] = GENMALLOC((f3d->row[i]) * sizeof(int));
			for (unsigned int j = 0U; (int)j < f3d->row[i]; j++) {
				tnnfifo[i][j] = GENMALLOC(1 * sizeof(char[MAX_NAME_CHARS]));
				tfnamefifo[i][j] =
					GENMALLOC(1 * sizeof(char[MAX_NAME_CHARS]));
				if (!((j == f3d->row[f3d->col - 1] - 1) &&
				      (i == (f3d->col - 1)))) {
					strlcpy(tnnfifo[i][j], f3d->nnfifo[i][j], MAX_NAME_CHARS);
					strlcpy(tfnamefifo[i][j],
						f3d->fnamefifo[i][j], MAX_NAME_CHARS);
					tbiasfifo[i][j] = f3d->biasfifo[i][j];
					tfidfifo[i][j] = f3d->fidfifo[i][j];
				}
			}
		}

		/*
		 * Copy data from old raw data to new memory location
		 */
		strlcpy(tnnfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1], entry,
			MAX_NAME_CHARS);
		strlcpy(tfnamefifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1],
			"none", MAX_NAME_CHARS);
		tbiasfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = 0;
		tfidfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = 0;

		/*
		 * Free the old raw data structres
		 */
		for (unsigned int i = 0U; (int)i < f3d->col; i++) {
			for (unsigned int j = 0U; (int)j < f3d->row[i]; j++) {
				if (!((i == f3d->col - 1) &&
				      (j == f3d->row[i] - 1))) {
					GENFREE(f3d->nnfifo[i][j]);
					GENFREE(f3d->fnamefifo[i][j]);
				}
			}
			GENFREE(f3d->nnfifo[i]);
			GENFREE(f3d->fnamefifo[i]);
			GENFREE(f3d->biasfifo[i]);
			GENFREE(f3d->fidfifo[i]);
		}
		GENFREE(f3d->nnfifo);
		GENFREE(f3d->fnamefifo);
		GENFREE(f3d->biasfifo);
		GENFREE(f3d->fidfifo);

		/*
		 * Point to new data
		 */
		f3d->nnfifo = tnnfifo;
		f3d->fnamefifo = tfnamefifo;
		f3d->biasfifo = tbiasfifo;
		f3d->fidfifo = tfidfifo;
	}
}
