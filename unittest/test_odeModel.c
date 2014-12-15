/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
#include "unittest.h"

#include <sbmlsolver/odeModel.h>
#include <sbmlsolver/solverError.h>

/* fixtures */
static odeModel_t *model;

static void teardown_model(void)
{
	ODEModel_free(model);
}

static ASTNode_t *det;

static void teardown_determinant(void)
{
	ASTNode_free(det);
}

/* helpers */
#define CHECK_VI_EQ(vi, expected) do {			\
		const ASTNode_t *node;					\
		char *actual;							\
		node = ODEModel_getOde(model, (vi));	\
		ck_assert(node != NULL);				\
		actual = SBML_formulaToString(node);	\
		ck_assert_str_eq(actual, (expected));	\
		free(actual);							\
	} while (0)

#define CHECK_VI_ASSIGNMENT(vi, expected) do {		\
		const ASTNode_t *node;						\
		char *actual;								\
		node = ODEModel_getAssignment(model, (vi));	\
		ck_assert(node != NULL);					\
		actual = SBML_formulaToString(node);		\
		ck_assert_str_eq(actual, (expected));		\
		free(actual);								\
	} while (0)

#define CHECK_VARIABLEINDEX(i, name) do {								\
		variableIndex_t *vi;											\
		vi = ODEModel_getVariableIndexByNum(model, (i));				\
		ck_assert(vi != NULL);											\
		ck_assert_str_eq(VariableIndex_getName(vi, model), (name));		\
		ck_assert(VariableIndex_getName(vi, model) == ODEModel_getVariableName(model, vi));	\
		ck_assert_int_eq(VariableIndex_getIndex(vi), (i));				\
		VariableIndex_free(vi);											\
	} while (0)

#define CHECK_EQ(i, name, expected) do {								\
		variableIndex_t *vi;											\
		vi = ODEModel_getVariableIndexByNum(model, (i));				\
		ck_assert(vi != NULL);											\
		ck_assert_str_eq(VariableIndex_getName(vi, model), (name));		\
		ck_assert(VariableIndex_getName(vi, model) == ODEModel_getVariableName(model, vi));	\
		ck_assert_int_eq(VariableIndex_getIndex(vi), (i));				\
		CHECK_VI_EQ(vi, expected);										\
		VariableIndex_free(vi);											\
	} while (0)

#define CHECK_ASSIGNMENT(i, name, expected) do {						\
		variableIndex_t *vi;											\
		vi = ODEModel_getVariableIndexByNum(model, (i));				\
		ck_assert(vi != NULL);											\
		ck_assert_str_eq(VariableIndex_getName(vi, model), (name));		\
		ck_assert(VariableIndex_getName(vi, model) == ODEModel_getVariableName(model, vi));	\
		ck_assert_int_eq(VariableIndex_getIndex(vi), (i));				\
		CHECK_VI_ASSIGNMENT(vi, expected);								\
		VariableIndex_free(vi);											\
	} while (0)

#define CHECK_NO_ERROR() \
	ck_assert_int_eq(SolverError_getNum(ERROR_ERROR_TYPE), 0)

#define CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY() do {						\
		ck_assert_int_eq(SolverError_getNum(ERROR_ERROR_TYPE), 1);		\
		ck_assert_str_eq(SolverError_getMessage(ERROR_ERROR_TYPE, 0),	\
						 "Cyclic dependency found in topological sorting."); \
		SolverError_clear();											\
	} while (0)

#define CHECK_JACOBI_ELEMENT(n, expected_i, expected_j) do {	\
		const nonzeroElem_t *nze;								\
		nze = ODEModel_getJacobiElement(model, (n));			\
		ck_assert(nze != NULL);									\
		ck_assert_int_eq(nze->i, (expected_i));					\
		ck_assert_int_eq(nze->j, (expected_j));					\
	} while (0)

#define CHECK_DETERMINANT(expected) do {		\
		char *actual;							\
		actual = SBML_formulaToString(det);		\
		ck_assert_str_eq(actual, (expected));	\
		free(actual);							\
	} while (0)

/* test cases */
START_TEST(test_ODEModel_createFromFile_MAPK)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("MAPK.xml"));
	ck_assert(model != NULL);
	ck_assert_int_eq(ODEModel_getNalg(model), 0);
	ck_assert_int_eq(ODEModel_getNeq(model), 8);
	ck_assert_int_eq(ODEModel_getNumAssignments(model), 10);
	ck_assert_int_eq(ODEModel_getNumConstants(model), 4);
	ck_assert_int_eq(ODEModel_getNumValues(model), 22);
	ck_assert_int_eq(ODEModel_hasVariable(model, "no_such_variable"), 0);
	CHECK_EQ(0, "MKKK", "(J1 - J0) / uVol");
	CHECK_EQ(1, "MKKK_P", "(J0 - J1) / uVol");
	CHECK_EQ(2, "MKK", "(J5 - J2) / uVol");
	CHECK_EQ(3, "MKK_P", "(J2 - J3 + J4 - J5) / uVol");
	CHECK_EQ(4, "MKK_PP", "(J3 - J4) / uVol");
	CHECK_EQ(5, "MAPK", "(J9 - J6) / uVol");
	CHECK_EQ(6, "MAPK_P", "(J6 - J7 + J8 - J9) / uVol");
	CHECK_EQ(7, "MAPK_PP", "(J7 - J8) / uVol");
	CHECK_ASSIGNMENT(8, "J0", "V1 * MKKK / ((1 + (MAPK_PP / Ki)^1) * (K1 + MKKK))");
	CHECK_ASSIGNMENT(9, "J1", "0.25 * MKKK_P / (8 + MKKK_P)");
	CHECK_ASSIGNMENT(10, "J2", "0.025 * MKKK_P * MKK / (15 + MKK)");
	CHECK_ASSIGNMENT(11, "J3", "0.025 * MKKK_P * MKK_P / (15 + MKK_P)");
	CHECK_ASSIGNMENT(12, "J4", "0.75 * MKK_PP / (15 + MKK_PP)");
	CHECK_ASSIGNMENT(13, "J5", "0.75 * MKK_P / (15 + MKK_P)");
	CHECK_ASSIGNMENT(14, "J6", "0.025 * MKK_PP * MAPK / (15 + MAPK)");
	CHECK_ASSIGNMENT(15, "J7", "0.025 * MKK_PP * MAPK_P / (15 + MAPK_P)");
	CHECK_ASSIGNMENT(16, "J8", "0.5 * MAPK_PP / (15 + MAPK_PP)");
	CHECK_ASSIGNMENT(17, "J9", "0.5 * MAPK_P / (15 + MAPK_P)");
	CHECK_VARIABLEINDEX(18, "uVol");
	CHECK_VARIABLEINDEX(19, "V1");
	CHECK_VARIABLEINDEX(20, "Ki");
	CHECK_VARIABLEINDEX(21, "K1");
	ck_assert(!ODEModel_hasCycle(model));
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeODEs(model), 10);
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeEvents(model), 0);
}
END_TEST

START_TEST(test_ODEModel_createFromFile_basic_model1_forward_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("basic-model1-forward-l2.xml"));
	ck_assert(model != NULL);
	ck_assert_int_eq(ODEModel_getNalg(model), 0);
	ck_assert_int_eq(ODEModel_getNeq(model), 2);
	ck_assert_int_eq(ODEModel_getNumAssignments(model), 2);
	ck_assert_int_eq(ODEModel_getNumConstants(model), 1);
	ck_assert_int_eq(ODEModel_getNumValues(model), 5);
	ck_assert_int_eq(ODEModel_hasVariable(model, "no_such_variable"), 0);
	CHECK_EQ(0, "S1", "(R2 - R1) / c");
	CHECK_EQ(1, "S2", "(R1 - R2) / c");
	CHECK_ASSIGNMENT(2, "R1", "1 * S1");
	CHECK_ASSIGNMENT(3, "R2", "0 * S2");
	CHECK_VARIABLEINDEX(4, "c");
	ck_assert(!ODEModel_hasCycle(model));
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeODEs(model), 2);
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeEvents(model), 0);
}
END_TEST

START_TEST(test_ODEModel_createFromFile_basic)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("basic.xml"));
	ck_assert(model != NULL);
	ck_assert_int_eq(ODEModel_getNalg(model), 0);
	ck_assert_int_eq(ODEModel_getNeq(model), 2);
	ck_assert_int_eq(ODEModel_getNumAssignments(model), 2);
	ck_assert_int_eq(ODEModel_getNumConstants(model), 2);
	ck_assert_int_eq(ODEModel_getNumValues(model), 6);
	ck_assert_int_eq(ODEModel_hasVariable(model, "no_such_variable"), 0);
	CHECK_EQ(0, "S1", "(R2 - R1) / c");
	CHECK_EQ(1, "S2", "(R1 - R2) / c");
	CHECK_ASSIGNMENT(2, "R1", "k_1 * S1");
	CHECK_ASSIGNMENT(3, "R2", "0 * S2");
	CHECK_VARIABLEINDEX(4, "c");
	CHECK_VARIABLEINDEX(5, "k_1");
	ck_assert(!ODEModel_hasCycle(model));
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeODEs(model), 2);
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeEvents(model), 0);
}
END_TEST

START_TEST(test_ODEModel_createFromFile_events_1_event_1_assignment_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("events-1-event-1-assignment-l2.xml"));
	ck_assert(model != NULL);
	ck_assert_int_eq(ODEModel_getNalg(model), 0);
	ck_assert_int_eq(ODEModel_getNeq(model), 2);
	ck_assert_int_eq(ODEModel_getNumAssignments(model), 1);
	ck_assert_int_eq(ODEModel_getNumConstants(model), 1);
	ck_assert_int_eq(ODEModel_getNumValues(model), 4);
	ck_assert_int_eq(ODEModel_hasVariable(model, "no_such_variable"), 0);
	CHECK_EQ(0, "S1", "-(R / compartment)");
	CHECK_EQ(1, "S2", "R / compartment");
	CHECK_ASSIGNMENT(2, "R", "S1");
	CHECK_VARIABLEINDEX(3, "compartment");
	ck_assert(!ODEModel_hasCycle(model));
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeODEs(model), 1);
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeEvents(model), 0);
}
END_TEST

START_TEST(test_ODEModel_createFromFile_events_2_events_1_assignment_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("events-2-events-1-assignment-l2.xml"));
	ck_assert(model != NULL);
	ck_assert_int_eq(ODEModel_getNalg(model), 0);
	ck_assert_int_eq(ODEModel_getNeq(model), 2);
	ck_assert_int_eq(ODEModel_getNumAssignments(model), 1);
	ck_assert_int_eq(ODEModel_getNumConstants(model), 1);
	ck_assert_int_eq(ODEModel_getNumValues(model), 4);
	ck_assert_int_eq(ODEModel_hasVariable(model, "no_such_variable"), 0);
	CHECK_EQ(0, "S1", "-(R / compartment)");
	CHECK_EQ(1, "S2", "R / compartment");
	CHECK_ASSIGNMENT(2, "R", "S1");
	CHECK_VARIABLEINDEX(3, "compartment");
	ck_assert(!ODEModel_hasCycle(model));
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeODEs(model), 1);
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeEvents(model), 0);
}
END_TEST

START_TEST(test_ODEModel_createFromFile_huang96)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("huang96.xml"));
	ck_assert(model != NULL);
	ck_assert_int_eq(ODEModel_getNalg(model), 0);
	ck_assert_int_eq(ODEModel_getNeq(model), 22);
	ck_assert_int_eq(ODEModel_getNumAssignments(model), 20);
	ck_assert_int_eq(ODEModel_getNumConstants(model), 1);
	ck_assert_int_eq(ODEModel_getNumValues(model), 43);
	ck_assert_int_eq(ODEModel_hasVariable(model, "no_such_variable"), 0);
	CHECK_EQ(0, "E1", "(r1b - r1a) / compartment");
	CHECK_EQ(1, "E2", "(r2b - r2a) / compartment");
	CHECK_EQ(2, "KKK", "(r2b - r1a) / compartment");
	CHECK_EQ(3, "P_KKK", "(r1b - r2a - r3a + r3b - r5a + r5b) / compartment");
	CHECK_EQ(4, "KK", "(r4b - r3a) / compartment");
	CHECK_EQ(5, "P_KK", "(r3b - r4a - r5a + r6b) / compartment");
	CHECK_EQ(6, "PP_KK", "(r5b - r6a - r7a + r7b - r9a + r9b) / compartment");
	CHECK_EQ(7, "K", "(r8b - r7a) / compartment");
	CHECK_EQ(8, "P_K", "(r7b - r8a - r9a + r10b) / compartment");
	CHECK_EQ(9, "PP_K", "(r9b - r10a) / compartment");
	CHECK_EQ(10, "KPase", "(r8b - r8a - r10a + r10b) / compartment");
	CHECK_EQ(11, "KKPase", "(r4b - r4a - r6a + r6b) / compartment");
	CHECK_EQ(12, "E1_KKK", "(r1a - r1b) / compartment");
	CHECK_EQ(13, "E2_P_KKK", "(r2a - r2b) / compartment");
	CHECK_EQ(14, "P_KKK_KK", "(r3a - r3b) / compartment");
	CHECK_EQ(15, "P_KKK_P_KK", "(r5a - r5b) / compartment");
	CHECK_EQ(16, "PP_KK_K", "(r7a - r7b) / compartment");
	CHECK_EQ(17, "PP_KK_P_K", "(r9a - r9b) / compartment");
	CHECK_EQ(18, "KKPase_PP_KK", "(r6a - r6b) / compartment");
	CHECK_EQ(19, "KKPase_P_KK", "(r4a - r4b) / compartment");
	CHECK_EQ(20, "KPase_PP_K", "(r10a - r10b) / compartment");
	CHECK_EQ(21, "KPase_P_K", "(r8a - r8b) / compartment");
	CHECK_ASSIGNMENT(22, "r1a", "1000 * E1 * KKK - 150 * E1_KKK");
	CHECK_ASSIGNMENT(23, "r1b", "150 * E1_KKK");
	CHECK_ASSIGNMENT(24, "r2a", "1000 * E2 * P_KKK - 150 * E2_P_KKK");
	CHECK_ASSIGNMENT(25, "r2b", "150 * E2_P_KKK");
	CHECK_ASSIGNMENT(26, "r3a", "1000 * KK * P_KKK - 150 * P_KKK_KK");
	CHECK_ASSIGNMENT(27, "r3b", "150 * P_KKK_KK");
	CHECK_ASSIGNMENT(28, "r4a", "1000 * P_KK * KKPase - 150 * KKPase_P_KK");
	CHECK_ASSIGNMENT(29, "r4b", "150 * KKPase_P_KK");
	CHECK_ASSIGNMENT(30, "r5a", "1000 * P_KK * P_KKK - 150 * P_KKK_P_KK");
	CHECK_ASSIGNMENT(31, "r5b", "150 * P_KKK_P_KK");
	CHECK_ASSIGNMENT(32, "r6a", "1000 * PP_KK * KKPase - 150 * KKPase_PP_KK");
	CHECK_ASSIGNMENT(33, "r6b", "150 * KKPase_PP_KK");
	CHECK_ASSIGNMENT(34, "r7a", "1000 * K * PP_KK - 150 * PP_KK_K");
	CHECK_ASSIGNMENT(35, "r7b", "150 * PP_KK_K");
	CHECK_ASSIGNMENT(36, "r8a", "1000 * P_K * KPase - 150 * KPase_P_K");
	CHECK_ASSIGNMENT(37, "r8b", "150 * KPase_P_K");
	CHECK_ASSIGNMENT(38, "r9a", "1000 * P_K * PP_KK - 150 * PP_KK_P_K");
	CHECK_ASSIGNMENT(39, "r9b", "150 * PP_KK_P_K");
	CHECK_ASSIGNMENT(40, "r10a", "1000 * PP_K * KPase - 150 * KPase_PP_K");
	CHECK_ASSIGNMENT(41, "r10b", "150 * KPase_PP_K");
	CHECK_VARIABLEINDEX(42, "compartment");
	ck_assert(!ODEModel_hasCycle(model));
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeODEs(model), 20);
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeEvents(model), 0);
}
END_TEST

START_TEST(test_ODEModel_createFromFile_repressilator)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("repressilator.xml"));
	ck_assert(model != NULL);
	ck_assert_int_eq(ODEModel_getNalg(model), 0);
	ck_assert_int_eq(ODEModel_getNeq(model), 6);
	ck_assert_int_eq(ODEModel_getNumAssignments(model), 0);
	ck_assert_int_eq(ODEModel_getNumConstants(model), 4);
	ck_assert_int_eq(ODEModel_getNumValues(model), 10);
	ck_assert_int_eq(ODEModel_hasVariable(model, "no_such_variable"), 0);
	CHECK_EQ(0, "x1", "beta * (y1 - x1)");
	CHECK_EQ(1, "x2", "beta * (y2 - x2)");
	CHECK_EQ(2, "x3", "beta * (y3 - x3)");
	CHECK_EQ(3, "y1", "alpha * x1 / (1 + x1 + rho * x3) - y1");
	CHECK_EQ(4, "y2", "alpha * x2 / (1 + x2 + rho * x1) - y2");
	CHECK_EQ(5, "y3", "alpha * x3 / (1 + x3 + rho * x2) - y3");
	CHECK_VARIABLEINDEX(6, "compartment");
	CHECK_VARIABLEINDEX(7, "alpha");
	CHECK_VARIABLEINDEX(8, "beta");
	CHECK_VARIABLEINDEX(9, "rho");
	ck_assert(!ODEModel_hasCycle(model));
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeODEs(model), 0);
	ck_assert_int_eq(ODEModel_getNumAssignmentsBeforeEvents(model), 0);
}
END_TEST

START_TEST(test_topoSort_1)
{
	static int *matrix[1];
	static int row[1];
	List_t *r;
	int *p;

	matrix[0] = row;

	row[0] = 0;
	r = topoSort(matrix, 1, NULL, NULL);
	ck_assert(r != NULL);
	ck_assert(List_size(r) == 1);
	p = List_get(r, 0);
	ck_assert_int_eq(*p, 0);
	free(p);
	List_free(r);
	CHECK_NO_ERROR();

	row[0] = 1;
	r = topoSort(matrix, 1, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();
}
END_TEST

START_TEST(test_topoSort_2)
{
	static int *matrix[2];
	static int row0[2];
	static int row1[2];
	List_t *r;
	int *p;

	matrix[0] = row0;
	matrix[1] = row1;

	row0[0] = 0;
	row0[1] = 0;
	row1[0] = 0;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r != NULL);
	ck_assert(List_size(r) == 2);
	p = List_get(r, 0);
	ck_assert_int_eq(*p, 0);
	free(p);
	p = List_get(r, 1);
	ck_assert_int_eq(*p, 1);
	free(p);
	List_free(r);
	CHECK_NO_ERROR();

	row0[0] = 0;
	row0[1] = 0;
	row1[0] = 0;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 0;
	row0[1] = 0;
	row1[0] = 1;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r != NULL);
	ck_assert(List_size(r) == 2);
	p = List_get(r, 0);
	ck_assert_int_eq(*p, 0);
	free(p);
	p = List_get(r, 1);
	ck_assert_int_eq(*p, 1);
	free(p);
	List_free(r);
	CHECK_NO_ERROR();

	row0[0] = 0;
	row0[1] = 0;
	row1[0] = 1;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 0;
	row0[1] = 1;
	row1[0] = 0;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r != NULL);
	ck_assert(List_size(r) == 2);
	p = List_get(r, 0);
	ck_assert_int_eq(*p, 1);
	free(p);
	p = List_get(r, 1);
	ck_assert_int_eq(*p, 0);
	free(p);
	List_free(r);
	CHECK_NO_ERROR();

	row0[0] = 0;
	row0[1] = 1;
	row1[0] = 0;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 0;
	row0[1] = 1;
	row1[0] = 1;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 0;
	row0[1] = 1;
	row1[0] = 1;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 0;
	row1[0] = 0;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 0;
	row1[0] = 0;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 0;
	row1[0] = 1;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 0;
	row1[0] = 1;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 1;
	row1[0] = 0;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 1;
	row1[0] = 0;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 1;
	row1[0] = 1;
	row1[1] = 0;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();

	row0[0] = 1;
	row0[1] = 1;
	row1[0] = 1;
	row1[1] = 1;
	r = topoSort(matrix, 2, NULL, NULL);
	ck_assert(r == NULL);
	CHECK_ERROR_ABOUT_CYCLIC_DEPENDENCY();
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_NULL)
{
	ck_assert_int_eq(ODEModel_constructJacobian(NULL), 0);
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_MAPK)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("MAPK.xml"));
	ck_assert_int_eq(model->jacobian, 0);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	ck_assert_int_eq(model->jacobian, 1);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_getNumJacobiElements(model), 26);
	CHECK_JACOBI_ELEMENT(0, 0, 0);
	CHECK_JACOBI_ELEMENT(1, 0, 1);
	CHECK_JACOBI_ELEMENT(2, 0, 7);
	CHECK_JACOBI_ELEMENT(3, 1, 0);
	CHECK_JACOBI_ELEMENT(4, 1, 1);
	CHECK_JACOBI_ELEMENT(5, 1, 7);
	CHECK_JACOBI_ELEMENT(6, 2, 1);
	CHECK_JACOBI_ELEMENT(7, 2, 2);
	CHECK_JACOBI_ELEMENT(8, 2, 3);
	CHECK_JACOBI_ELEMENT(9, 3, 1);
	CHECK_JACOBI_ELEMENT(10, 3, 2);
	CHECK_JACOBI_ELEMENT(11, 3, 3);
	CHECK_JACOBI_ELEMENT(12, 3, 4);
	CHECK_JACOBI_ELEMENT(13, 4, 1);
	CHECK_JACOBI_ELEMENT(14, 4, 3);
	CHECK_JACOBI_ELEMENT(15, 4, 4);
	CHECK_JACOBI_ELEMENT(16, 5, 4);
	CHECK_JACOBI_ELEMENT(17, 5, 5);
	CHECK_JACOBI_ELEMENT(18, 5, 6);
	CHECK_JACOBI_ELEMENT(19, 6, 4);
	CHECK_JACOBI_ELEMENT(20, 6, 5);
	CHECK_JACOBI_ELEMENT(21, 6, 6);
	CHECK_JACOBI_ELEMENT(22, 6, 7);
	CHECK_JACOBI_ELEMENT(23, 7, 4);
	CHECK_JACOBI_ELEMENT(24, 7, 6);
	CHECK_JACOBI_ELEMENT(25, 7, 7);
	ODEModel_freeJacobian(model);
	ck_assert_int_eq(model->jacobian, 0);
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_basic_model1_forward_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("basic-model1-forward-l2.xml"));
	ck_assert_int_eq(model->jacobian, 0);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	ck_assert_int_eq(model->jacobian, 1);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_getNumJacobiElements(model), 2);
	CHECK_JACOBI_ELEMENT(0, 0, 0);
	CHECK_JACOBI_ELEMENT(1, 1, 0);
	ODEModel_freeJacobian(model);
	ck_assert_int_eq(model->jacobian, 0);
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_basic)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("basic.xml"));
	ck_assert_int_eq(model->jacobian, 0);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	ck_assert_int_eq(model->jacobian, 1);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_getNumJacobiElements(model), 2);
	CHECK_JACOBI_ELEMENT(0, 0, 0);
	CHECK_JACOBI_ELEMENT(1, 1, 0);
	ODEModel_freeJacobian(model);
	ck_assert_int_eq(model->jacobian, 0);
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_events_1_event_1_assignment_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("events-1-event-1-assignment-l2.xml"));
	ck_assert_int_eq(model->jacobian, 0);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	ck_assert_int_eq(model->jacobian, 1);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_getNumJacobiElements(model), 2);
	CHECK_JACOBI_ELEMENT(0, 0, 0);
	CHECK_JACOBI_ELEMENT(1, 1, 0);
	ODEModel_freeJacobian(model);
	ck_assert_int_eq(model->jacobian, 0);
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_events_2_events_1_assignment_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("events-2-events-1-assignment-l2.xml"));
	ck_assert_int_eq(model->jacobian, 0);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	ck_assert_int_eq(model->jacobian, 1);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_getNumJacobiElements(model), 2);
	CHECK_JACOBI_ELEMENT(0, 0, 0);
	CHECK_JACOBI_ELEMENT(1, 1, 0);
	ODEModel_freeJacobian(model);
	ck_assert_int_eq(model->jacobian, 0);
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_huang96)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("huang96.xml"));
	ck_assert_int_eq(model->jacobian, 0);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	ck_assert_int_eq(model->jacobian, 1);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_getNumJacobiElements(model), 92);
	ODEModel_freeJacobian(model);
	ck_assert_int_eq(model->jacobian, 0);
}
END_TEST

START_TEST(test_ODEModel_constructJacobian_repressilator)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("repressilator.xml"));
	ck_assert_int_eq(model->jacobian, 0);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	ck_assert_int_eq(model->jacobian, 1);
	ck_assert_int_eq(model->jacobianFailed, 0);
	ck_assert_int_eq(ODEModel_getNumJacobiElements(model), 15);
	CHECK_JACOBI_ELEMENT(0, 0, 0);
	CHECK_JACOBI_ELEMENT(1, 0, 3);
	CHECK_JACOBI_ELEMENT(2, 1, 1);
	CHECK_JACOBI_ELEMENT(3, 1, 4);
	CHECK_JACOBI_ELEMENT(4, 2, 2);
	CHECK_JACOBI_ELEMENT(5, 2, 5);
	CHECK_JACOBI_ELEMENT(6, 3, 0);
	CHECK_JACOBI_ELEMENT(7, 3, 2);
	CHECK_JACOBI_ELEMENT(8, 3, 3);
	CHECK_JACOBI_ELEMENT(9, 4, 0);
	CHECK_JACOBI_ELEMENT(10, 4, 1);
	CHECK_JACOBI_ELEMENT(11, 4, 4);
	CHECK_JACOBI_ELEMENT(12, 5, 1);
	CHECK_JACOBI_ELEMENT(13, 5, 2);
	CHECK_JACOBI_ELEMENT(14, 5, 5);
	ODEModel_freeJacobian(model);
	ck_assert_int_eq(model->jacobian, 0);
}
END_TEST

START_TEST(test_ODEModel_constructDeterminant_MAPK)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("MAPK.xml"));
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	det = ODEModel_constructDeterminant(model);
	ck_assert(det != NULL);
	/* too big to check the determinant */
}
END_TEST

START_TEST(test_ODEModel_constructDeterminant_basic_model1_forward_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("basic-model1-forward-l2.xml"));
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	det = ODEModel_constructDeterminant(model);
	ck_assert(det != NULL);
	CHECK_DETERMINANT(""); /* TODO: is it really expected? */
}
END_TEST

START_TEST(test_ODEModel_constructDeterminant_basic)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("basic.xml"));
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	det = ODEModel_constructDeterminant(model);
	ck_assert(det != NULL);
	CHECK_DETERMINANT(""); /* TODO: is it really expected? */
}
END_TEST

START_TEST(test_ODEModel_constructDeterminant_events_1_event_1_assignment_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("events-1-event-1-assignment-l2.xml"));
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	det = ODEModel_constructDeterminant(model);
	ck_assert(det != NULL);
	CHECK_DETERMINANT(""); /* TODO: is it really expected? */
}
END_TEST

START_TEST(test_ODEModel_constructDeterminant_events_2_events_1_assignment_l2)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("events-2-events-1-assignment-l2.xml"));
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	det = ODEModel_constructDeterminant(model);
	ck_assert(det != NULL);
	CHECK_DETERMINANT(""); /* TODO: is it really expected? */
}
END_TEST

START_TEST(test_ODEModel_constructDeterminant_huang96)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("huang96.xml"));
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
#if 0 /* seems too big to construct its determinant */
	det = ODEModel_constructDeterminant(model);
	ck_assert(det != NULL);
#else
	det = NULL;
#endif
}
END_TEST

START_TEST(test_ODEModel_constructDeterminant_repressilator)
{
	model = ODEModel_createFromFile(EXAMPLES_FILENAME("repressilator.xml"));
	ck_assert_int_eq(ODEModel_constructJacobian(model), 1);
	det = ODEModel_constructDeterminant(model);
	ck_assert(det != NULL);
	CHECK_DETERMINANT(" - beta * ( - beta * ( - beta * ( - ( + 1)) + alpha * x1 "
					  "/ (1 + x1 + rho * x3)^2 * rho *  - (alpha / (1 + x3 + rho"
					  " * x2) - alpha * x3 / (1 + x3 + rho * x2)^2) * ( +  + bet"
					  "a)) - (alpha / (1 + x2 + rho * x1) - alpha * x2 / (1 + x2"
					  " + rho * x1)^2) * ( + beta * ( + ( - beta)) - alpha * x1 "
					  "/ (1 + x1 + rho * x3)^2 * rho *  - (alpha / (1 + x3 + rho"
					  " * x2) - alpha * x3 / (1 + x3 + rho * x2)^2) * ( - ( + be"
					  "ta * beta))) - alpha * x3 / (1 + x3 + rho * x2)^2 * rho *"
					  " ( + beta * ( + ) - alpha * x1 / (1 + x1 + rho * x3)^2 * "
					  "rho * )) - (alpha / (1 + x1 + rho * x3) - alpha * x1 / (1"
					  " + x1 + rho * x3)^2) * ( + beta * ( + beta * ( + beta * ("
					  " + 1)) - (alpha / (1 + x3 + rho * x2) - alpha * x3 / (1 +"
					  " x3 + rho * x2)^2) * ( + beta * ( + beta))) - (alpha / (1"
					  " + x2 + rho * x1) - alpha * x2 / (1 + x2 + rho * x1)^2) *"
					  " ( - beta * ( + beta * ( - beta)) - (alpha / (1 + x3 + rh"
					  "o * x2) - alpha * x3 / (1 + x3 + rho * x2)^2) * ( + beta "
					  "* ( + beta * beta))) - alpha * x3 / (1 + x3 + rho * x2)^2"
					  " * rho * ( - beta * ( + beta * ))) - alpha * x2 / (1 + x2"
					  " + rho * x1)^2 * rho * ( + beta * ( + beta * ( + beta *  "
					  "+ ) - alpha * x1 / (1 + x1 + rho * x3)^2 * rho * ( + beta"
					  " * ) - (alpha / (1 + x3 + rho * x2) - alpha * x3 / (1 + x"
					  "3 + rho * x2)^2) * ( + beta *  - )) - alpha * x3 / (1 + x"
					  "3 + rho * x2)^2 * rho * ( - beta * ( + beta *  - ) + alph"
					  "a * x1 / (1 + x1 + rho * x3)^2 * rho * ( + beta * ( + bet"
					  "a * beta))))");
}
END_TEST

/* public */
Suite *create_suite_odeModel(void)
{
	Suite *s;
	TCase *tc_ODEModel_createFromFile;
	TCase *tc_topoSort;
	TCase *tc_ODEModel_constructJacobian;
	TCase *tc_ODEModel_constructDeterminant;

	s = suite_create("odeModel");

	tc_ODEModel_createFromFile = tcase_create("ODEModel_createFromFile");
	tcase_add_checked_fixture(tc_ODEModel_createFromFile,
							  NULL,
							  teardown_model);
	tcase_add_test(tc_ODEModel_createFromFile, test_ODEModel_createFromFile_MAPK);
	tcase_add_test(tc_ODEModel_createFromFile, test_ODEModel_createFromFile_basic_model1_forward_l2);
	tcase_add_test(tc_ODEModel_createFromFile, test_ODEModel_createFromFile_basic);
	tcase_add_test(tc_ODEModel_createFromFile, test_ODEModel_createFromFile_events_1_event_1_assignment_l2);
	tcase_add_test(tc_ODEModel_createFromFile, test_ODEModel_createFromFile_events_2_events_1_assignment_l2);
	tcase_add_test(tc_ODEModel_createFromFile, test_ODEModel_createFromFile_huang96);
	tcase_add_test(tc_ODEModel_createFromFile, test_ODEModel_createFromFile_repressilator);
	suite_add_tcase(s, tc_ODEModel_createFromFile);

	tc_topoSort = tcase_create("topoSort");
	tcase_add_test(tc_topoSort, test_topoSort_1);
	tcase_add_test(tc_topoSort, test_topoSort_2);
	suite_add_tcase(s, tc_topoSort);

	tc_ODEModel_constructJacobian = tcase_create("ODEModel_constructJacobian");
	tcase_add_checked_fixture(tc_ODEModel_constructJacobian,
							  NULL,
							  teardown_model);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_NULL);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_MAPK);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_basic_model1_forward_l2);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_basic);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_events_1_event_1_assignment_l2);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_events_2_events_1_assignment_l2);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_huang96);
	tcase_add_test(tc_ODEModel_constructJacobian, test_ODEModel_constructJacobian_repressilator);
	suite_add_tcase(s, tc_ODEModel_constructJacobian);

	tc_ODEModel_constructDeterminant = tcase_create("ODEModel_constructDeterminant");
	tcase_add_checked_fixture(tc_ODEModel_constructDeterminant,
							  NULL,
							  teardown_model);
	tcase_add_checked_fixture(tc_ODEModel_constructDeterminant,
							  NULL,
							  teardown_determinant);
	tcase_add_test(tc_ODEModel_constructDeterminant, test_ODEModel_constructDeterminant_MAPK);
	tcase_add_test(tc_ODEModel_constructDeterminant, test_ODEModel_constructDeterminant_basic_model1_forward_l2);
	tcase_add_test(tc_ODEModel_constructDeterminant, test_ODEModel_constructDeterminant_basic);
	tcase_add_test(tc_ODEModel_constructDeterminant, test_ODEModel_constructDeterminant_events_1_event_1_assignment_l2);
	tcase_add_test(tc_ODEModel_constructDeterminant, test_ODEModel_constructDeterminant_events_2_events_1_assignment_l2);
	tcase_add_test(tc_ODEModel_constructDeterminant, test_ODEModel_constructDeterminant_huang96);
	tcase_add_test(tc_ODEModel_constructDeterminant, test_ODEModel_constructDeterminant_repressilator);
	suite_add_tcase(s, tc_ODEModel_constructDeterminant);

	return s;
}