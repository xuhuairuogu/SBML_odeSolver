/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
#ifndef _UNITTEST_UNITTEST_H_
#define _UNITTEST_UNITTEST_H_

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>

#define EXAMPLES_FILENAME0(dirname, basename) (#dirname "/" basename)
#define EXAMPLES_FILENAME1(dirname, basename) EXAMPLES_FILENAME0(dirname, basename)
#define EXAMPLES_FILENAME(basename) EXAMPLES_FILENAME1(EXAMPLES, basename)

#define CHECK_PARAMETER(m, i, expected) do {				\
		Parameter_t *p = Model_getParameter((m), (i));		\
		ck_assert(p != NULL);								\
		ck_assert_str_eq(Parameter_getId(p), (expected));	\
	} while (0)

#define CHECK_REACTION(m, i, id, formula) do {					\
		Reaction_t *r;											\
		KineticLaw_t *k;										\
		r = Model_getReaction((m), (i));						\
		ck_assert(r != NULL);									\
		ck_assert_str_eq(Reaction_getId(r), (id));				\
		k = Reaction_getKineticLaw(r);							\
		ck_assert(k != NULL);									\
		ck_assert_str_eq(KineticLaw_getFormula(k), (formula));	\
	} while (0)

#define CHECK_RULE(m, i, expected) do {						\
		Rule_t *r = Model_getRule((m), (i));				\
		ck_assert(r != NULL);								\
		ck_assert_str_eq(Rule_getFormula(r), (expected));	\
	} while (0)

#define CHECK_DOUBLE_WITH_TOLERANCE(d, expected) ck_assert(fabs((d) - (expected)) <= DBL_EPSILON)

Suite *create_suite_ASTIndexNameNode(void);
Suite *create_suite_charBuffer(void);
Suite *create_suite_interpol(void);
Suite *create_suite_modelSimplify(void);
Suite *create_suite_odeConstruct(void);
Suite *create_suite_odeModel(void);
Suite *create_suite_processAST(void);
Suite *create_suite_sbml(void);
Suite *create_suite_sbmlResults(void);
Suite *create_suite_solverError(void);
Suite *create_suite_util(void);

#endif
