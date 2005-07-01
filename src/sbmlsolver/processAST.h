/*
  Last changed Time-stamp: <2005-06-30 14:07:03 raim>
  $Id: processAST.h,v 1.2 2005/07/01 12:53:40 raimc Exp $
*/
#ifndef _PROCESSAST_H_
#define _PROCESSAST_H_

/* libSBML header files */
#include <sbml/SBMLTypes.h>

/* own header files */
#include "sbmlsolver/cvodedata.h"

#define SQR(x) ((x)*(x))
#define SQRT(x) pow((x),(.5))
/* Helper Macros to get the second or the third child
   of an Abstract Syntax Tree */
#define child(x,y)  ASTNode_getChild(x,y)
#define child2(x,y,z)  ASTNode_getChild(ASTNode_getChild(x,y),z)
#define child3(x,y,z,w) ASTNode_getChild(ASTNode_getChild(ASTNode_getChild(x,y),z),w)

ASTNode_t *
copyAST(const ASTNode_t *f);
double
evaluateAST(ASTNode_t *n, CvodeData data);
ASTNode_t *
differentiateAST(ASTNode_t *f, char*x);
ASTNode_t *
determinantNAST(ASTNode_t ***A, int N);  
ASTNode_t *
AST_simplify(ASTNode_t *f);
ASTNode_t *
simplifyAST(ASTNode_t *f);
void
setUserDefinedFunction(double(*udf)(char*, int, double*));

#endif

/* End of file */
