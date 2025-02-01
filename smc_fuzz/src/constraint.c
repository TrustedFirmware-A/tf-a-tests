/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arg_struct_def.h>
#include <constraint.h>
#include <field_specification.h>

#include <debug.h>

#ifdef SMC_FUZZ_TMALLOC
#define GENMALLOC(x)    malloc((x))
#define GENFREE(x)      free((x))
#else
#define GENMALLOC(x)    smcmalloc((x), mmod)
#define GENFREE(x)      smcfree((x), mmod)
#endif

/*******************************************************
* Random 64 bit generator for registers
*******************************************************/

uint64_t rand64bit(void)
{
	uint64_t xreg = (rand() % (1 << FUZZ_MAX_SHIFT_AMNT)) << FUZZ_MAX_SHIFT_AMNT;

	xreg = ((rand() % (1 << FUZZ_MAX_SHIFT_AMNT)) | xreg) << FUZZ_MAX_SHIFT_AMNT;
	xreg = ((rand() % (1 << FUZZ_MAX_SHIFT_AMNT)) | xreg) << FUZZ_MAX_SHIFT_AMNT;
	xreg = ((rand() % (1 << FUZZ_MAX_SHIFT_AMNT)) | xreg);
	return xreg;
}

/*******************************************************
* Shift left function for registers
*******************************************************/

uint64_t shiftlft(uint64_t val, int shamnt)
{
	uint64_t ressh = val;

	if (shamnt > FUZZ_MAX_REG_SIZE) {
		printf("Error: cannot shift beyond %d bits\n", FUZZ_MAX_REG_SIZE);
		panic();
	}
	if (shamnt > FUZZ_MAX_SHIFT_AMNT) {
		for (int i = 0; i < ((shamnt / FUZZ_MAX_SHIFT_AMNT) + 1); i++) {
			if (i == (shamnt / FUZZ_MAX_SHIFT_AMNT)) {
				ressh = ressh << (shamnt % FUZZ_MAX_SHIFT_AMNT);
			} else {
				ressh = ressh << FUZZ_MAX_SHIFT_AMNT;
			}
		}
	} else {
		ressh = ressh << shamnt;
	}
	return ressh;
}

/*******************************************************
* Shift right function for registers
*******************************************************/

uint64_t shiftrht(uint64_t val, int shamnt)
{
	uint64_t ressh = val;

	if (shamnt > FUZZ_MAX_REG_SIZE) {
		printf("Error: cannot shift beyond %d bits\n", FUZZ_MAX_REG_SIZE);
		panic();
	}
	if (shamnt > FUZZ_MAX_SHIFT_AMNT) {
		for (int i = 0; i < ((shamnt / FUZZ_MAX_SHIFT_AMNT) + 1); i++) {
			if (i == (shamnt / FUZZ_MAX_SHIFT_AMNT)) {
				ressh = ressh >> (shamnt % FUZZ_MAX_SHIFT_AMNT);
			} else {
				ressh = ressh >> FUZZ_MAX_SHIFT_AMNT;
			}
		}
	} else {
		ressh = ressh >> shamnt;
	}
	return ressh;
}

/*******************************************************
* Set constraints for the fields in the SMC call
*******************************************************/


void setconstraint(int contype, uint64_t *vecinput, int veclen, int fieldnameptr, struct memmod *mmod, int mode)
{
	int argdef = fuzzer_fieldarg[fieldnameptr];
	int fieldname = fuzzer_fieldfld[fieldnameptr];

	if ((argdef > MAX_ARG_LENGTH) || (argdef < 0)) {
		printf("SMC argument is out of bounds\n");
		panic();
	}
	if ((fieldname > (fuzzer_arg_array_lst[argdef].arg_span[1] -
		fuzzer_arg_array_lst[argdef].arg_span[0])) || (fieldname < 0)) {
		printf("SMC fieldname is out of bounds\n");
		panic();
	}
	int fieldptr = fuzzer_arg_array_lst[argdef].arg_span[0] + fieldname;

	if ((contype > FUZZER_CONSTRAINT_VECTOR) || (contype < 0)) {
		printf("SMC constraint type is out of bounds\n");
		panic();
	}
	if (mode > 2) {
		printf("SMC constriant mode input is invalid\n");
		panic();
	}
	if (mmod == NULL) {
		printf("SMC constraint memory pointer is invalid\n");
		panic();
	}
	if (contype == FUZZER_CONSTRAINT_SVALUE) {
		if (veclen < 1) {
			printf("vector length to constraint for single value is not large enough");
			printf(" %d", veclen);
			panic();
		}
		if (vecinput == NULL) {
			printf("vector input to constraint single value is not defined\n");
			panic();
		}
		if (fuzzer_arg_array[fieldptr].contval == NULL) {
			fuzzer_arg_array[fieldptr].contval = GENMALLOC(1 * sizeof(uint64_t **));
			fuzzer_arg_array[fieldptr].contval[0] = GENMALLOC(1 * sizeof(uint64_t *));
			fuzzer_arg_array[fieldptr].contval[0][0] = vecinput[0];
			fuzzer_arg_array[fieldptr].contvallen = GENMALLOC(1 * sizeof(int *));
			fuzzer_arg_array[fieldptr].contvallen[0] = 1;
			fuzzer_arg_array[fieldptr].contlen = 1;
			fuzzer_arg_array[fieldptr].conttype = GENMALLOC(1 * sizeof(int *));
			fuzzer_arg_array[fieldptr].conttype[0] = contype;
		} else {
			if (mode == FUZZER_CONSTRAINT_ACCMODE) {
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					if (fuzzer_arg_array[fieldptr].conttype[i] ==
					FUZZER_CONSTRAINT_SVALUE) {
						if (fuzzer_arg_array[fieldptr].contval[i][0] == vecinput[0]) {
							return;
						}
					}
				}
				uint64_t **tarray;

				tarray = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen)
				* sizeof(uint64_t **));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarray[i] = GENMALLOC(fuzzer_arg_array[fieldptr].contvallen[i]
					* sizeof(uint64_t *));
					for (int k = 0; k < fuzzer_arg_array[fieldptr].contvallen[i]; k++) {
						tarray[i][k] = fuzzer_arg_array[fieldptr].contval[i][k];
					}
				}
				tarray[fuzzer_arg_array[fieldptr].contlen] = GENMALLOC(1 * sizeof(int *));
				tarray[fuzzer_arg_array[fieldptr].contlen][0] = vecinput[0];
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					GENFREE(fuzzer_arg_array[fieldptr].contval[i]);
				}
				GENFREE(fuzzer_arg_array[fieldptr].contval);
				fuzzer_arg_array[fieldptr].contval = tarray;
				int *tarraysingle;

				tarraysingle = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen) * sizeof(int *));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarraysingle[i] = fuzzer_arg_array[fieldptr].contvallen[i];
				}
				tarraysingle[fuzzer_arg_array[fieldptr].contlen] = 1;
				GENFREE(fuzzer_arg_array[fieldptr].contvallen);
				fuzzer_arg_array[fieldptr].contvallen = tarraysingle;
				tarraysingle = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen)
				* sizeof(int *));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarraysingle[i] = fuzzer_arg_array[fieldptr].conttype[i];
				}
				tarraysingle[fuzzer_arg_array[fieldptr].contlen] = contype;
				GENFREE(fuzzer_arg_array[fieldptr].conttype);
				fuzzer_arg_array[fieldptr].conttype = tarraysingle;
				fuzzer_arg_array[fieldptr].contlen++;
			}
			if (mode == FUZZER_CONSTRAINT_EXCMODE) {
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					GENFREE(fuzzer_arg_array[fieldptr].contval[i]);
				}
				GENFREE(fuzzer_arg_array[fieldptr].contval);
				GENFREE(fuzzer_arg_array[fieldptr].contvallen);
				GENFREE(fuzzer_arg_array[fieldptr].conttype);
				fuzzer_arg_array[fieldptr].contval = GENMALLOC(1 * sizeof(uint64_t **));
				fuzzer_arg_array[fieldptr].contval[0] = GENMALLOC(1 * sizeof(uint64_t *));
				fuzzer_arg_array[fieldptr].contval[0][0] = vecinput[0];
				fuzzer_arg_array[fieldptr].contvallen = GENMALLOC(1 * sizeof(int *));
				fuzzer_arg_array[fieldptr].contvallen[0] = 1;
				fuzzer_arg_array[fieldptr].contlen = 1;
				fuzzer_arg_array[fieldptr].conttype = GENMALLOC(1 * sizeof(int *));
				fuzzer_arg_array[fieldptr].conttype[0] = contype;
			}
		}
	}
	if (contype == FUZZER_CONSTRAINT_RANGE) {
		if (veclen < 2) {
			printf("vector length to constraint for range is not large enough");
			printf(" %d", veclen);
			panic();
		}
		if (vecinput == NULL) {
			printf("vector inputs to constraint for range is not defined\n");
			panic();
		}
		if (fuzzer_arg_array[fieldptr].contval == NULL) {
			fuzzer_arg_array[fieldptr].contval = GENMALLOC(1 * sizeof(uint64_t **));
			fuzzer_arg_array[fieldptr].contval[0] = GENMALLOC(2 * sizeof(uint64_t *));
			fuzzer_arg_array[fieldptr].contval[0][0] = vecinput[0];
			fuzzer_arg_array[fieldptr].contval[0][1] = vecinput[1];
			fuzzer_arg_array[fieldptr].contvallen = GENMALLOC(1 * sizeof(int *));
			fuzzer_arg_array[fieldptr].contvallen[0] = 2;
			fuzzer_arg_array[fieldptr].contlen = 1;
			fuzzer_arg_array[fieldptr].conttype = GENMALLOC(1 * sizeof(int *));
			fuzzer_arg_array[fieldptr].conttype[0] = contype;
		} else {
			if (mode == FUZZER_CONSTRAINT_ACCMODE) {
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					if (fuzzer_arg_array[fieldptr].conttype[i] ==
					FUZZER_CONSTRAINT_RANGE) {
						if ((fuzzer_arg_array[fieldptr].contval[i][0] ==
						vecinput[0]) && (fuzzer_arg_array[fieldptr].contval[i][1]
						== vecinput[1])) {
							return;
						}
					}
				}
				uint64_t **tarray;

				tarray = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen)
				* sizeof(uint64_t **));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarray[i] =
					GENMALLOC(fuzzer_arg_array[fieldptr].contvallen[i]
					* sizeof(uint64_t *));
					for (int k = 0; k < fuzzer_arg_array[fieldptr].contvallen[i]; k++) {
						tarray[i][k] = fuzzer_arg_array[fieldptr].contval[i][k];
					}
				}
				tarray[fuzzer_arg_array[fieldptr].contlen] = GENMALLOC(2
				* sizeof(uint64_t *));
				tarray[fuzzer_arg_array[fieldptr].contlen][0] = vecinput[0];
				tarray[fuzzer_arg_array[fieldptr].contlen][1] = vecinput[1];
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					GENFREE(fuzzer_arg_array[fieldptr].contval[i]);
				}
				GENFREE(fuzzer_arg_array[fieldptr].contval);
				fuzzer_arg_array[fieldptr].contval = tarray;
				int *tarraysingle;
				tarraysingle =
				GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen) * sizeof(int *));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarraysingle[i] = fuzzer_arg_array[fieldptr].contvallen[i];
				}
				tarraysingle[fuzzer_arg_array[fieldptr].contlen] = 2;
				GENFREE(fuzzer_arg_array[fieldptr].contvallen);
				fuzzer_arg_array[fieldptr].contvallen = tarraysingle;
				tarraysingle = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen)
				* sizeof(int *));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarraysingle[i] = fuzzer_arg_array[fieldptr].conttype[i];
				}
				tarraysingle[fuzzer_arg_array[fieldptr].contlen] = contype;
				GENFREE(fuzzer_arg_array[fieldptr].conttype);
				fuzzer_arg_array[fieldptr].conttype = tarraysingle;
				fuzzer_arg_array[fieldptr].contlen++;
			}
			if (mode == FUZZER_CONSTRAINT_EXCMODE) {
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					GENFREE(fuzzer_arg_array[fieldptr].contval[i]);
				}
				GENFREE(fuzzer_arg_array[fieldptr].contval);
				GENFREE(fuzzer_arg_array[fieldptr].contvallen);
				GENFREE(fuzzer_arg_array[fieldptr].conttype);
				fuzzer_arg_array[fieldptr].contval = GENMALLOC(1 * sizeof(uint64_t **));
				fuzzer_arg_array[fieldptr].contval[0] = GENMALLOC(2 * sizeof(uint64_t *));
				fuzzer_arg_array[fieldptr].contval[0][0] = vecinput[0];
				fuzzer_arg_array[fieldptr].contval[0][1] = vecinput[1];
				fuzzer_arg_array[fieldptr].contvallen = GENMALLOC(1 * sizeof(int *));
				fuzzer_arg_array[fieldptr].contvallen[0] = 2;
				fuzzer_arg_array[fieldptr].contlen = 1;
				fuzzer_arg_array[fieldptr].conttype = GENMALLOC(1 * sizeof(int *));
				fuzzer_arg_array[fieldptr].conttype[0] = contype;
			}
		}
	}
	if (contype == FUZZER_CONSTRAINT_VECTOR) {
		if (veclen < 2) {
			printf("vector length to constraint for vector is not large enough");
			printf(" %d", veclen);
			panic();
		}
		if (vecinput == NULL) {
			printf("vector input to constraint vector is not defined\n");
			panic();
		}
		if (fuzzer_arg_array[fieldptr].contval == NULL) {
			fuzzer_arg_array[fieldptr].contval = GENMALLOC(1 * sizeof(uint64_t **));
			fuzzer_arg_array[fieldptr].contval[0] = GENMALLOC(veclen * sizeof(uint64_t *));
			for (int i = 0; i < veclen; i++) {
				fuzzer_arg_array[fieldptr].contval[0][i] = vecinput[i];
			}
			fuzzer_arg_array[fieldptr].contvallen = GENMALLOC(1 * sizeof(int *));
			fuzzer_arg_array[fieldptr].contvallen[0] = veclen;
			fuzzer_arg_array[fieldptr].contlen = 1;
			fuzzer_arg_array[fieldptr].conttype = GENMALLOC(1 * sizeof(int *));
			fuzzer_arg_array[fieldptr].conttype[0] = contype;

		} else {
			if (mode == FUZZER_CONSTRAINT_ACCMODE) {
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					if (fuzzer_arg_array[fieldptr].conttype[i] ==
					FUZZER_CONSTRAINT_VECTOR) {
						if (fuzzer_arg_array[fieldptr].contvallen[i] == veclen) {
							int fne = 0;
							for (int j = 0; j <
							fuzzer_arg_array[fieldptr].contvallen[i]; j++) {
							if (fuzzer_arg_array[fieldptr].contval
								[i][j] != vecinput[j]) {
									fne = 1;
								}
							}
							if (fne == 0) {
								return;
							}
						}
					}
				}
				uint64_t **tarray;

				tarray = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen)
				* sizeof(uint64_t **));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarray[i] = GENMALLOC(fuzzer_arg_array[fieldptr].contvallen
					[i] * sizeof(uint64_t *));
					for (int k = 0; k < fuzzer_arg_array[fieldptr].contvallen[i]; k++) {
						tarray[i][k] = fuzzer_arg_array[fieldptr].contval[i][k];
					}
				}
				tarray[fuzzer_arg_array[fieldptr].contlen] =
				GENMALLOC(veclen * sizeof(uint64_t *));
				for (int i = 0; i < veclen; i++) {
					tarray[fuzzer_arg_array[fieldptr].contlen][i] = vecinput[i];
				}
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					GENFREE(fuzzer_arg_array[fieldptr].contval[i]);
				}
				GENFREE(fuzzer_arg_array[fieldptr].contval);
				fuzzer_arg_array[fieldptr].contval = tarray;
				int *tarraysingle;
				tarraysingle = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen)
				* sizeof(int *));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarraysingle[i] = fuzzer_arg_array[fieldptr].contvallen[i];
				}
				tarraysingle[fuzzer_arg_array[fieldptr].contlen] = veclen;
				GENFREE(fuzzer_arg_array[fieldptr].contvallen);
				fuzzer_arg_array[fieldptr].contvallen = tarraysingle;
				tarraysingle = GENMALLOC((1 + fuzzer_arg_array[fieldptr].contlen)
				* sizeof(int *));
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					tarraysingle[i] = fuzzer_arg_array[fieldptr].conttype[i];
				}
				tarraysingle[fuzzer_arg_array[fieldptr].contlen] = contype;
				GENFREE(fuzzer_arg_array[fieldptr].conttype);
				fuzzer_arg_array[fieldptr].conttype = tarraysingle;
				fuzzer_arg_array[fieldptr].contlen++;
			}
			if (mode == FUZZER_CONSTRAINT_EXCMODE) {
				for (int i = 0; i < fuzzer_arg_array[fieldptr].contlen; i++) {
					GENFREE(fuzzer_arg_array[fieldptr].contval[i]);
				}
				GENFREE(fuzzer_arg_array[fieldptr].contval);
				GENFREE(fuzzer_arg_array[fieldptr].contvallen);
				GENFREE(fuzzer_arg_array[fieldptr].conttype);
				fuzzer_arg_array[fieldptr].contval = GENMALLOC(1 * sizeof(uint64_t **));
				fuzzer_arg_array[fieldptr].contval[0] = GENMALLOC(veclen
				* sizeof(uint64_t *));
				for (int i = 0; i < veclen; i++) {
					fuzzer_arg_array[fieldptr].contval[0][i] = vecinput[i];
				}
				fuzzer_arg_array[fieldptr].contvallen = GENMALLOC(1 * sizeof(int *));
				fuzzer_arg_array[fieldptr].contvallen[0] = veclen;
				fuzzer_arg_array[fieldptr].contlen = 1;
				fuzzer_arg_array[fieldptr].conttype = GENMALLOC(1 * sizeof(int *));
				fuzzer_arg_array[fieldptr].conttype[0] = contype;
			}
		}
	}
}

/*******************************************************
* Generate the uncondition(no constraint)
* fields in the SMC call
*******************************************************/

uint64_t generate_field_uncon(int smccall, int rsel)
{
	uint64_t shiftreg = 0;
	uint64_t resreg = 0;
	int fieldptr = 0;
	int argptr = fuzzer_arg_array_start[smccall] + rsel;

	for (int i = 0; i <= (fuzzer_arg_array_lst[argptr].arg_span[1] -
	fuzzer_arg_array_lst[argptr].arg_span[0]); i++) {
		fieldptr = fuzzer_arg_array_lst[argptr].arg_span[0] + i;
		shiftreg = shiftlft((rand() % shiftlft(1, fuzzer_arg_array[fieldptr].bitw)),
		fuzzer_arg_array[fieldptr].bitst);
		resreg = resreg | shiftreg;
	}
	return resreg;
}

uint64_t generate_field_con(int smccall, int rsel)
{
	uint64_t shiftreg = 0;
	uint64_t resreg = 0;
	int fieldptr = 0;
	int nullstat = 0;
	int argptr = fuzzer_arg_array_start[smccall] + rsel;

	for (int i = 0; i <= (fuzzer_arg_array_lst[argptr].arg_span[1] -
	fuzzer_arg_array_lst[argptr].arg_span[0]); i++) {
		fieldptr = fuzzer_arg_array_lst[argptr].arg_span[0] + i;
		nullstat = 0;
		if (fuzzer_arg_array[fieldptr].contval == NULL) {
			if (fuzzer_arg_array[fieldptr].defval >
				(shiftlft(1, fuzzer_arg_array[fieldptr].bitw) - 1)) {
				printf("Default constraint will not fit inside bitfield %llx %llx\n",
				fuzzer_arg_array[fieldptr].defval,
				(shiftlft(1, fuzzer_arg_array[fieldptr].bitw) - 1));
				panic();
			} else {
				shiftreg = shiftlft(fuzzer_arg_array[fieldptr].defval,
				fuzzer_arg_array[fieldptr].bitst);
				resreg = resreg | shiftreg;
			}
			nullstat = 1;
		} else if (fuzzer_arg_array[fieldptr].contval[0] == NULL) {
			if (fuzzer_arg_array[fieldptr].defval >
				(shiftlft(1, fuzzer_arg_array[fieldptr].bitw) - 1)) {
				printf("Default constraint will not fit inside bitfield %llx %llx\n",
				fuzzer_arg_array[fieldptr].defval,
				(shiftlft(1, fuzzer_arg_array[fieldptr].bitw) - 1));
				panic();
			} else {
				shiftreg = shiftlft(fuzzer_arg_array[fieldptr].defval,
				fuzzer_arg_array[fieldptr].bitst);
				resreg = resreg | shiftreg;
			}
			nullstat = 1;
		}
		if (nullstat == 0) {
			int selcon = rand() % (fuzzer_arg_array[fieldptr].contlen);

			if (fuzzer_arg_array[fieldptr].conttype[selcon] == FUZZER_CONSTRAINT_SVALUE) {
				if (fuzzer_arg_array[fieldptr].contval[selcon][0] >
					((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) - 1)) {
					printf("Constraint will not fit inside bitfield %llx %llx\n",
					fuzzer_arg_array[fieldptr].contval[selcon][0],
					((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) - 1));
					panic();
				} else {
					shiftreg = shiftlft(fuzzer_arg_array[fieldptr].contval[selcon][0],
					fuzzer_arg_array[fieldptr].bitst);
					resreg = resreg | shiftreg;
				}
			}

			if (fuzzer_arg_array[fieldptr].conttype[selcon] == FUZZER_CONSTRAINT_RANGE) {
				uint64_t maxn = shiftlft(1, fuzzer_arg_array[fieldptr].bitw);

				if ((fuzzer_arg_array[fieldptr].contval[selcon][0] >
					((maxn) - 1)) || ((fuzzer_arg_array[fieldptr].contval[selcon][1] >
					((maxn) - 1))))  {
					if (fuzzer_arg_array[fieldptr].contval[selcon][0] >
					((maxn) - 1)) {
						printf("Constraint will not fit inside bitfield %llx %llx\n",
						fuzzer_arg_array[fieldptr].contval[selcon][0], ((maxn) - 1));
					}
					if (fuzzer_arg_array[fieldptr].contval[selcon][1] >
					((maxn) - 1)) {
						printf("Constraint will not fit inside bitfield %llx %llx\n",
						fuzzer_arg_array[fieldptr].contval[selcon][1], ((maxn) - 1));
					}
					panic();
				} else {
					shiftreg = shiftlft(((rand() %
					(fuzzer_arg_array[fieldptr].contval[selcon][1] -
					fuzzer_arg_array[fieldptr].contval[selcon][0] + 1)) +
					fuzzer_arg_array[fieldptr].contval[selcon][0]),
					fuzzer_arg_array[fieldptr].bitst);
					resreg = resreg | shiftreg;
				}
			}

			if (fuzzer_arg_array[fieldptr].conttype[selcon] == FUZZER_CONSTRAINT_VECTOR) {
				for (int j = 0; j < fuzzer_arg_array[fieldptr].contvallen[selcon]; j++) {
					if (fuzzer_arg_array[fieldptr].contval[selcon][j] >
						((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) - 1)) {
						printf("Constraint will not fit inside bitfield");
						printf(" %llx %llx\n",
						fuzzer_arg_array[fieldptr].contval[selcon][j],
						((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) - 1));
						panic();
					}
				}
				shiftreg = shiftlft((fuzzer_arg_array[fieldptr].contval[selcon]
				[(rand() % (fuzzer_arg_array[fieldptr].contvallen[selcon]))]),
				fuzzer_arg_array[fieldptr].bitst);
				resreg = resreg | shiftreg;
			}
		}
	}
	return resreg;
}

/*******************************************************
* Generate the field arguments for constrained fields
* for all sanity levels
*******************************************************/

struct inputparameters generate_args(int smccall, int sanity)
{
	if ((smccall > MAX_SMC_CALLS) || (smccall < 0)) {
		printf("generate args SMC call is out of bounds\n");
		panic();
	}
	if ((sanity > SANITY_LEVEL_3) || (sanity < 0)) {
		printf("generate args sanity level is out of bounds\n");
		panic();
	}
	struct inputparameters nparam;

	nparam.x1 = 1;
	if (sanity == SANITY_LEVEL_0) {
		for (int i = 0; i < fuzzer_arg_array_range[smccall]; i++) {
			switch (i) {
				case 0: {
					nparam.x1 = rand64bit();
					break;
				}
				case 1: {
					nparam.x2 = rand64bit();
					break;
				}
				case 2: {
					nparam.x3 = rand64bit();
					break;
				}
				case 3: {
					nparam.x4 = rand64bit();
					break;
				}
				case 4: {
					nparam.x5 = rand64bit();
					break;
				}
				case 5: {
					nparam.x6 = rand64bit();
					break;
				}
				case 6: {
					nparam.x7 = rand64bit();
					break;
				}
				case 7: {
					nparam.x8 = rand64bit();
					break;
				}
				case 8: {
					nparam.x9 = rand64bit();
					break;
				}
				case 9: {
					nparam.x10 = rand64bit();
					break;
				}
				case 10: {
					nparam.x11 = rand64bit();
					break;
				}
				case 11: {
					nparam.x12 = rand64bit();
					break;
				}
				case 12: {
					nparam.x13 = rand64bit();
					break;
				}
				case 13: {
					nparam.x14 = rand64bit();
					break;
				}
				case 14: {
					nparam.x15 = rand64bit();
					break;
				}
				case 15: {
					nparam.x16 = rand64bit();
					break;
				}
				case 16: {
					nparam.x17 = rand64bit();
					break;
				}
			}
		}
	}
	if (sanity == SANITY_LEVEL_1) {
		int selreg = rand() % (fuzzer_arg_array_range[smccall] + 1);
		for (int i = 0; i < fuzzer_arg_array_range[smccall]; i++) {
			switch (i) {
				case 0: {
					if (selreg == 0) {
						nparam.x1 = generate_field_uncon(smccall, i);
					} else {
						nparam.x1 = rand64bit();
					}
					break;
				}
				case 1: {
					if (selreg == 1) {
						nparam.x2 = generate_field_uncon(smccall, i);
					} else {
						nparam.x2 = rand64bit();
					}
					break;
				}
				case 2: {
					if (selreg == 2) {
						nparam.x3 = generate_field_uncon(smccall, i);
					} else {
						nparam.x3 = rand64bit();
					}
					break;
				}
				case 3: {
					if (selreg == 3) {
						nparam.x4 = generate_field_uncon(smccall, i);
					} else {
						nparam.x4 = rand64bit();
					}
					break;
				}
				case 4: {
					if (selreg == 4) {
						nparam.x5 = generate_field_uncon(smccall, i);
					} else {
						nparam.x5 = rand64bit();
					}
					break;
				}
				case 5: {
					if (selreg == 5) {
						nparam.x6 = generate_field_uncon(smccall, i);
					} else {
						nparam.x6 = rand64bit();
					}
					break;
				}
				case 6: {
					if (selreg == 6) {
						nparam.x7 = generate_field_uncon(smccall, i);
					} else {
						nparam.x7 = rand64bit();
					}
					break;
				}
				case 7: {
					if (selreg == 7) {
						nparam.x8 = generate_field_uncon(smccall, i);
					} else {
						nparam.x8 = rand64bit();
					}
					break;
				}
				case 8: {
					if (selreg == 8) {
						nparam.x9 = generate_field_uncon(smccall, i);
					} else {
						nparam.x9 = rand64bit();
					}
					break;
				}
				case 9: {
					if (selreg == 9) {
						nparam.x10 = generate_field_uncon(smccall, i);
					} else {
						nparam.x10 = rand64bit();
					}
					break;
				}
				case 10: {
					if (selreg == 10) {
						nparam.x11 = generate_field_uncon(smccall, i);
					} else {
						nparam.x11 = rand64bit();
					}
					break;
				}
				case 11: {
					if (selreg == 11) {
						nparam.x12 = generate_field_uncon(smccall, i);
					} else {
						nparam.x12 = rand64bit();
					}
					break;
				}
				case 12: {
					if (selreg == 12) {
						nparam.x13 = generate_field_uncon(smccall, i);
					} else {
						nparam.x13 = rand64bit();
					}
					break;
				}
				case 13: {
					if (selreg == 13) {
						nparam.x14 = generate_field_uncon(smccall, i);
					} else {
						nparam.x14 = rand64bit();
					}
					break;
				}
				case 14: {
					if (selreg == 14) {
						nparam.x15 = generate_field_uncon(smccall, i);
					} else {
						nparam.x15 = rand64bit();
					}
					break;
				}
				case 15: {
					if (selreg == 15) {
						nparam.x16 = generate_field_uncon(smccall, i);
					} else {
						nparam.x16 = rand64bit();
					}
					break;
				}
				case 16: {
					if (selreg == 16) {
						nparam.x17 = generate_field_uncon(smccall, i);
					} else {
						nparam.x17 = rand64bit();
					}
					break;
				}
			}
		}
	}
	if (sanity == SANITY_LEVEL_2) {
		for (int i = 0; i < fuzzer_arg_array_range[smccall]; i++) {
			switch (i) {
				case 0: {
					nparam.x1 = generate_field_uncon(smccall, i);
					break;
				}
				case 1: {
					nparam.x2 = generate_field_uncon(smccall, i);
					break;
				}
				case 2: {
					nparam.x3 = generate_field_uncon(smccall, i);
					break;
				}
				case 3: {
					nparam.x4 = generate_field_uncon(smccall, i);
					break;
				}
				case 4: {
					nparam.x5 = generate_field_uncon(smccall, i);
					break;
				}
				case 5: {
					nparam.x6 = generate_field_uncon(smccall, i);
					break;
				}
				case 6: {
					nparam.x7 = generate_field_uncon(smccall, i);
					break;
				}
				case 7: {
					nparam.x8 = generate_field_uncon(smccall, i);
					break;
				}
				case 8: {
					nparam.x9 = generate_field_uncon(smccall, i);
					break;
				}
				case 9: {
					nparam.x10 = generate_field_uncon(smccall, i);
					break;
				}
				case 10: {
					nparam.x11 = generate_field_uncon(smccall, i);
					break;
				}
				case 11: {
					nparam.x12 = generate_field_uncon(smccall, i);
					break;
				}
				case 12: {
					nparam.x13 = generate_field_uncon(smccall, i);
					break;
				}
				case 13: {
					nparam.x14 = generate_field_uncon(smccall, i);
					break;
				}
				case 14: {
					nparam.x15 = generate_field_uncon(smccall, i);
					break;
				}
				case 15: {
					nparam.x16 = generate_field_uncon(smccall, i);
					break;
				}
				case 16: {
					nparam.x17 = generate_field_uncon(smccall, i);
					break;
				}
			}
		}
	}
	if (sanity == SANITY_LEVEL_3) {
		for (int i = 0; i < fuzzer_arg_array_range[smccall]; i++) {
			switch (i) {
				case 0: {
					nparam.x1 = generate_field_con(smccall, i);
					break;
				}
				case 1: {
					nparam.x2 = generate_field_con(smccall, i);
					break;
				}
				case 2: {
					nparam.x3 = generate_field_con(smccall, i);
					break;
				}
				case 3: {
					nparam.x4 = generate_field_con(smccall, i);
					break;
				}
				case 4: {
					nparam.x5 = generate_field_con(smccall, i);
					break;
				}
				case 5: {
					nparam.x6 = generate_field_con(smccall, i);
					break;
				}
				case 6: {
					nparam.x7 = generate_field_con(smccall, i);
					break;
				}
				case 7: {
					nparam.x8 = generate_field_con(smccall, i);
					break;
				}
				case 8: {
					nparam.x9 = generate_field_con(smccall, i);
					break;
				}
				case 9: {
					nparam.x10 = generate_field_con(smccall, i);
					break;
				}
				case 10: {
					nparam.x11 = generate_field_con(smccall, i);
					break;
				}
				case 11: {
					nparam.x12 = generate_field_con(smccall, i);
					break;
				}
				case 12: {
					nparam.x13 = generate_field_con(smccall, i);
					break;
				}
				case 13: {
					nparam.x14 = generate_field_con(smccall, i);
					break;
				}
				case 14: {
					nparam.x15 = generate_field_con(smccall, i);
					break;
				}
				case 15: {
					nparam.x16 = generate_field_con(smccall, i);
					break;
				}
				case 16: {
					nparam.x17 = generate_field_con(smccall, i);
					break;
				}
			}
		}
	}
	#ifdef SMC_FUZZER_DEBUG
		print_smccall(smccall, nparam);
	#endif
	return nparam;
}

/*******************************************************
* Get generated value from fuzzer for a given field
*******************************************************/

uint64_t get_generated_value(int fieldnameptr, struct inputparameters inp)
{
	uint64_t xval = 0;
	int argdef = fuzzer_fieldarg[fieldnameptr];
	int fieldname = fuzzer_fieldfld[fieldnameptr];
	int fieldptr = fuzzer_arg_array_lst[argdef].arg_span[0] + fieldname;

	switch(fuzzer_arg_array[fieldptr].regnum) {
		case 1: {
			xval = shiftrht(inp.x1, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 2: {
			xval = shiftrht(inp.x2, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 3: {
			xval = shiftrht(inp.x3, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 4: {
			xval = shiftrht(inp.x4, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 5: {
			xval = shiftrht(inp.x5, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 6: {
			xval = shiftrht(inp.x6, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 7: {
			xval = shiftrht(inp.x7, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 8: {
			xval = shiftrht(inp.x8, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 9: {
			xval = shiftrht(inp.x9, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 10: {
			xval = shiftrht(inp.x10, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 11: {
			xval = shiftrht(inp.x11, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 12: {
			xval = shiftrht(inp.x12, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 13: {
			xval = shiftrht(inp.x13, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 14: {
			xval = shiftrht(inp.x14, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 15: {
			xval = shiftrht(inp.x15, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 16: {
			xval = shiftrht(inp.x16, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
		case 17: {
			xval = shiftrht(inp.x17, fuzzer_arg_array[fieldptr].bitst) &
			((shiftlft(1, fuzzer_arg_array[fieldptr].bitw)) -  1);
			return xval;
		}
	}
	return xval;
}

/*******************************************************
* Print the values from a generated SMC call from fuzzer
*******************************************************/

void print_smccall(int smccall, struct inputparameters inp)
{
	if ((smccall > MAX_SMC_CALLS) || (smccall < 0)) {
		printf("generate args SMC call is out of bounds\n");
		panic();
	}
	int argptr = fuzzer_arg_array_start[smccall];
	int fieldptr = fuzzer_arg_array_lst[fuzzer_arg_array_start[smccall]].arg_span[0];

	printf("%s\n", fuzzer_arg_array[fieldptr].smcname);
	for (int i = 0; i < (fuzzer_arg_array_range[smccall]); i++) {
		fieldptr = fuzzer_arg_array_lst[argptr + i].arg_span[0];
		printf("argument: %s\n", fuzzer_arg_array[fieldptr].smcargname);
		for (int j = fieldptr; j <= ((fuzzer_arg_array_lst[argptr + i].arg_span[1] -
		fuzzer_arg_array_lst[argptr + i].arg_span[0]) + fieldptr); j++) {
			printf("%s = %llx\n", fuzzer_arg_array[j].bnames,
				get_generated_value(j, inp));
		}
	}
	printf("\n\n");
}
