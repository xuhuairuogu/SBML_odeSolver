/*
  Last changed Time-stamp: <2007-09-07 16:56:17 raim>
  $Id: cvodeSolver.c,v 1.62 2007/09/07 18:12:55 raimc Exp $
*/
/* 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY, WITHOUT EVEN THE IMPLIED WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. The software and
 * documentation provided hereunder is on an "as is" basis, and the
 * authors have no obligations to provide maintenance, support,
 * updates, enhancements or modifications.  In no event shall the
 * authors be liable to any party for direct, indirect, special,
 * incidental or consequential damages, including lost profits, arising
 * out of the use of this software and its documentation, even if the
 * authors have been advised of the possibility of such damage.  See
 * the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * The original code contained here was initially developed by:
 *
 *     Rainer Machne
 *
 * Contributor(s):
 *     Andrew M. Finney
 *     Christoph Flamm
 */

/*! \defgroup cvode CVODES ODE Solver:  x(t)
  \ingroup integrator     
  \brief This module contains the functions that call SUNDIALS CVODES
  solver routines for stiff and non-stiff ODE systems
    

*/
/*@{*/

#include <stdio.h>
#include <stdlib.h>

/* Header Files for CVODE */
#include "cvodes/cvodes.h"    
#include "cvodes/cvodes_dense.h"
#include "nvector/nvector_serial.h"  

#include "sbmlsolver/cvodeData.h"
#include "sbmlsolver/processAST.h"
#include "sbmlsolver/odeModel.h"
#include "sbmlsolver/variableIndex.h"
#include "sbmlsolver/solverError.h"
#include "sbmlsolver/integratorInstance.h"
#include "sbmlsolver/cvodeSolver.h"
#include "sbmlsolver/sensSolver.h"
#include "sbmlsolver/modelSimplify.h"

static int fQ(realtype t, N_Vector y, N_Vector qdot, void *fQ_data);
static int f(realtype t, N_Vector y, N_Vector ydot, void *f_data);
static int JacODE(long int N, DenseMat J, realtype t,
		  N_Vector y, N_Vector fy, void *jac_data,
		  N_Vector vtemp1, N_Vector vtemp2, N_Vector vtemp3);

/** Calls CVODE to move the current simulation one time step.

produces appropriate error messages on failures and returns 1 if the
integration can continue, 0 otherwise. This function is called by
IntegratorInstance_integrateOneStep, but could also be called directly
by a calling application that is sure to use CVODES (and not e.g. IDA),
to avoid the if statements in the wrapper function.
*/

SBML_ODESOLVER_API int IntegratorInstance_cvodeOneStep(integratorInstance_t *engine)
{
  int i, flag;
  realtype *ydata = NULL;
    
  cvodeSolver_t *solver = engine->solver;
  cvodeData_t *data = engine->data;
  cvodeSettings_t *opt = engine->opt;
  odeModel_t *om = engine->om;

  if ( !engine->isValid )
  { 
    solver->t0 = solver->t;
    if ( !IntegratorInstance_createCVODESolverStructures(engine) )
    {
      fprintf(stderr, "engine not valid for unknown reasons, "
	      "please contact developers\n");
      return 0;
    }
  }

  if (!engine->clockStarted)
  {
    engine->startTime = clock();
    engine->clockStarted = 1 ;
  }

  /* Forward solver is only called if not in the adjoint (backward) phase */
  if( !opt->AdjointPhase )
  { 
    if( opt->DoAdjoint )
    {  
        /* CvodeF is needed in the forward phase if the adjoint soln is
	 desired  */  
        flag = CVodeF(solver->cvadj_mem, solver->tout,
		    solver->y, &(solver->t), CV_NORMAL, &(opt->ncheck));
     
    }
    else
    {
      /* calling CVODE */
       flag = CVode(solver->cvode_mem, solver->tout,
		    solver->y, &(solver->t), CV_NORMAL);
    }
   

    /*  if ( flag != CV_SUCCESS ) */
    if ( flag < CV_SUCCESS )
    {
      char *message[] =
	{
	  /*  0 CV_SUCCESS */
	  "Success",
	  /**/
	  /*  1 CV_ROOT_RETURN */
	  /*   "CVode succeeded, and found one or more roots" */
	  /*  2 CV_TSTOP_RETURN */
	  /*   "CVode succeeded and returned at tstop" */
	  /**/
	  /* -1 CV_MEM_NULL -1 (old CVODE_NO_MEM) */
	  "The cvode_mem argument was NULL",
	  /* -2 CV_ILL_INPUT */
	  "One of the inputs to CVode is illegal. This "
	  "includes the situation when a component of the "
	  "error weight vectors becomes < 0 during "
	  "internal time-stepping. The ILL_INPUT flag "
	  "will also be returned if the linear solver "
	  "routine CV--- (called by the user after "
	  "calling CVodeMalloc) failed to set one of the "
	  "linear solver-related fields in cvode_mem or "
	  "if the linear solver's init routine failed. In "
	  "any case, the user should see the printed "
	  "error message for more details.",
	  /* -3 CV_NO_MALLOC */
	  "cvode_mem was not allocated",
	  /* -4 CV_TOO_MUCH_WORK */
	  "The solver took %d internal steps but could not "
	  "compute variable values for time %g",
	  /* -5 CV_TOO_MUCH_ACC */
	  "The solver could not satisfy the accuracy " 
	  "requested for some internal step.",
	  /* -6 CV_ERR_FAILURE */
	  "Error test failures occurred too many times "
	  "during one internal time step or "
	  "occurred with |h| = hmin.",
	  /* -7 CV_CONV_FAILURE */
	  "Convergence test failures occurred too many "
	  "times during one internal time step or occurred "
	  "with |h| = hmin.",
	  /* -8 CV_LINIT_FAIL */
	  "CVode -- Initial Setup: "
	  "The linear solver's init routine failed.",
	  /* -9 CV_LSETUP_FAIL */
	  "The linear solver's setup routine failed in an "
	  "unrecoverable manner.",
	  /* -10 CV_LSOLVE_FAIL */
	  "The linear solver's solve routine failed in an "
	  "unrecoverable manner.",
	  /* -11 CV_MEM_FAIL */
	  "A memory allocation failed. "
	  "(including an attempt to increase maxord)",
	  /* -12 CV_RTFUNC_NULL */
	  "nrtfn > 0 but g = NULL.",
	  /* -13 CV_NO_SLDET */
	  "CVodeGetNumStabLimOrderReds -- Illegal attempt "
	  "to call without enabling SLDET.",
	  /* -14 CV_BAD_K */
	  "CVodeGetDky -- Illegal value for k.",
	  /* -15 CV_BAD_T */
	  "CVodeGetDky -- Illegal value for t.",
	  /* -16 CV_BAD_DKY */
	  "CVodeGetDky -- dky = NULL illegal.",
	  /* -17 CV_PDATA_NULL */
	  "???",
	};
	    

      SolverError_error(ERROR_ERROR_TYPE,
			flag,
			message[flag * -1],
			opt->Mxstep,
			solver->tout);
      SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_INTEGRATION_NOT_SUCCESSFUL,
			"Integration not successful. Results are not "
			"complete.");

      return 0 ; /* Error - stop integration*/
    }
    
    ydata = NV_DATA_S(solver->y);
    
    /* update cvodeData time dependent variables */    
    for ( i=0; i<om->neq; i++ )
      data->value[i] = ydata[i];

    /* update rest of data with internal default function */
    flag = IntegratorInstance_updateData(engine);
    if ( flag != 1 ){
      return 0;
    }    

  }
  /* if( !opt->AdjointPhase ) */
  else
  { /* AdjointPhase: */


    /* The adjoint engine*/
    flag = CVodeB(solver->cvadj_mem, solver->tout,
		  solver->yA, &(solver->t), CV_NORMAL);

  

    if ( flag <CV_SUCCESS  )
    {

   
      char *message[] =
	{
	  /*  0 CV_SUCCESS */
	  "Success",
	  /**/
	  /*  1 CV_ROOT_RETURN */
	  /*   "CVode succeeded, and found one or more roots" */
	  /*  2 CV_TSTOP_RETURN */
	  /*   "CVode succeeded and returned at tstop" */
	  /**/
	  /* -1 CV_MEM_NULL -1 (old CVODE_NO_MEM) */
	  "The cvode_mem argument was NULL",
	  /* -2 CV_ILL_INPUT */
	  "One of the inputs to CVode is illegal. This "
	  "includes the situation when a component of the "
	  "error weight vectors becomes < 0 during "
	  "internal time-stepping. The ILL_INPUT flag "
	  "will also be returned if the linear solver "
	  "routine CV--- (called by the user after "
	  "calling CVodeMalloc) failed to set one of the "
	  "linear solver-related fields in cvode_mem or "
	  "if the linear solver's init routine failed. In "
	  "any case, the user should see the printed "
	  "error message for more details.",
	  /* -3 CV_NO_MALLOC */
	  "cvode_mem was not allocated",
	  /* -4 CV_TOO_MUCH_WORK */
	  "The solver took %d internal steps but could not "
	  "compute variable values for time %g",
	  /* -5 CV_TOO_MUCH_ACC */
	  "The solver could not satisfy the accuracy " 
	  "requested for some internal step.",
	  /* -6 CV_ERR_FAILURE */
	  "Error test failures occurred too many times "
	  "during one internal time step or "
	  "occurred with |h| = hmin.",
	  /* -7 CV_CONV_FAILURE */
	  "Convergence test failures occurred too many "
	  "times during one internal time step or occurred "
	  "with |h| = hmin.",
	  /* -8 CV_LINIT_FAIL */
	  "CVode -- Initial Setup: "
	  "The linear solver's init routine failed.",
	  /* -9 CV_LSETUP_FAIL */
	  "The linear solver's setup routine failed in an "
	  "unrecoverable manner.",
	  /* -10 CV_LSOLVE_FAIL */
	  "The linear solver's solve routine failed in an "
	  "unrecoverable manner.",
	  /* -11 CV_MEM_FAIL */
	  "A memory allocation failed. "
	  "(including an attempt to increase maxord)",
	  /* -12 CV_RTFUNC_NULL */
	  "nrtfn > 0 but g = NULL.",
	  /* -13 CV_NO_SLDET */
	  "CVodeGetNumStabLimOrderReds -- Illegal attempt "
	  "to call without enabling SLDET.",
	  /* -14 CV_BAD_K */
	  "CVodeGetDky -- Illegal value for k.",
	  /* -15 CV_BAD_T */
	  "CVodeGetDky -- Illegal value for t.",
	  /* -16 CV_BAD_DKY */
	  "CVodeGetDky -- dky = NULL illegal.",
	  /* -17 CV_PDATA_NULL */
	  "Precond module not initialized",
          "Sensitivity index larger than number computed", 
	  "", 
	  "Quad integration not activated", 
	  "Forward sensitivity integration not achieved"
	};
    
	char *message2[] =
	  { "",
            "Cvadj_mem full", 
            "", 
            "Bad final time for adjoint problem",
	    "Memory for adjoint problem not created",
            "Reinit of forward failed at check point",
	    "Forward integration failed",
	    "Wrong task for adjoint integration",
	    "Output time outside forward problem interval",
	    "Wrong time in Hermite interpolation",
	  };
      

      if( flag > -100)
      {
	SolverError_error(ERROR_ERROR_TYPE,
			  flag,
			  message[flag * -1],
			  opt->Mxstep,
			  solver->tout);
        SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_INTEGRATION_NOT_SUCCESSFUL,
			"Integration not successful. Results are not "
			"complete.");
      }
      else
      {
	flag = flag + 100;
        SolverError_error(ERROR_ERROR_TYPE,
			  flag,
			  message2[flag * -1],
			  opt->Mxstep,
			  solver->tout);
        SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_INTEGRATION_NOT_SUCCESSFUL,
			"Integration not successful. Results are not "
			"complete.");

      }

      return 0 ; /* Error - stop integration*/
    }

    ydata = NV_DATA_S(solver->yA);
    
    /* update adjoint variables computed by CvodeS */    
    for ( i=0; i<om->neq; i++ )
      data->adjvalue[i] = ydata[i];

    /* update rest of adjoint data with internal default function */
    flag = IntegratorInstance_updateAdjData(engine);
    if ( flag != 1 ){
      fprintf(stderr, "update AdjData error!!\n");
      return 0;
    }

  }

  /*  calculating sensitivities */
  if ( opt->Sensitivity && !opt->AdjointPhase )
  { 
    flag = IntegratorInstance_getForwardSens(engine);
    if ( flag != 1 ) return 0;
    else return 1;
  }
  else if( opt->AdjointPhase )
  {
    flag = IntegratorInstance_getAdjSens(engine);
    if ( flag != 1 ) return 0;
    else return 1;
  }
  else
    return 1; /* OK */    
}





/************* CVODE integrator setup functions ************/


/** creates CVODE structures and fills cvodeSolver, 
    returns 1 on success or 0 on failure
*/
int
IntegratorInstance_createCVODESolverStructures(integratorInstance_t *engine)
{
  int i, flag, neq, method, iteration;
  odeModel_t *om = engine->om;
  cvodeData_t *data = engine->data;
  cvodeSolver_t *solver = engine->solver;
  cvodeSettings_t *opt = engine->opt;
  CVRhsFn rhsFunction;
  CVDenseJacFn jacODE = NULL;

 
  if ( !opt->AdjointPhase )
  {
    /* the main part: all allocations for the forward integrator */
    /* i.e, also for the forward phase of adjoint sens. analysis */

    neq = engine->om->neq; /* number of equations */

    if ( opt->compileFunctions )
    {
      rhsFunction = ODEModel_getCompiledCVODERHSFunction(om);
      if ( !rhsFunction ) return 0; /* error */
    }
    else
      rhsFunction = f ;
      
    if ( opt->UseJacobian )
    {
      if ( opt->compileFunctions )
      {
	jacODE = ODEModel_getCompiledCVODEJacobianFunction(om); 
	if ( !jacODE ) return 0; /* error */
      }
      else
	jacODE = JacODE;
    }

    /* CVODESolverStructures from former runs must be freed */
    /* if (  solver->y != NULL ) */
    /* 	IntegratorInstance_freeCVODEolverStructures(engine); */

    /**
     * Allocate y, abstol vectors
     */
    /*!!! valgrind memcheck adj_sensitivity: 560 (32 direct, 528 indirect)
      bytes  in 2 blocks are definitely lost !!!*/
    if ( solver->y == NULL )
    {
      solver->y = N_VNew_Serial(neq);
      CVODE_HANDLE_ERROR((void *)solver->y, "N_VNew_Serial for y", 0);
    }

    /*!!! valgrind memcheck sensitivity:   576 (32 direct, 544 indirect)
      bytes in 2 blocks are definitely lost !!!*/
    if ( solver->abstol == NULL )
    {
      solver->abstol = N_VNew_Serial(neq);
      CVODE_HANDLE_ERROR((void *)solver->abstol,
			 "N_VNew_Serial for abstol", 0);
    }

    /**
     * Initialize y, abstol vectors
     */
    for ( i=0; i<neq; i++ )
    {
      /* Set initial value vector components of y and y' */
      NV_Ith_S(solver->y, i) = data->value[i];

      /* Set absolute tolerance vector components,
	 currently the same absolute error is used for all y */
     /*  abstoldata[i] = opt->Error; */
      NV_Ith_S(solver->abstol, i) = opt->Error;
    }

    /* scalar relative tolerance: the same for all y */
    solver->reltol = opt->RError;

    /**
     * Call CVodeCreate to create the non-linear solver memory:\n
     *
     * Nonlinear Solver:\n
     * CV_BDF         Backward Differentiation Formula method\n
     * CV_ADAMS       Adams-Moulton method\n
     * Iteration Method:\n
     * CV_NEWTON      Newton iteration method\n
     * CV_FUNCTIONAL  functional iteration method\n
     */
    if ( opt->CvodeMethod == 1 ) method = CV_ADAMS;
    else method = CV_BDF;
    
    if ( opt->IterMethod == 1 ) iteration = CV_FUNCTIONAL;
    else iteration = CV_NEWTON;

    /* !!! valgrind memcheck sensitivity: 20,632 (1,880 direct,
       18,752 indirect) bytes in 1 blocks are definitely lost !!! */
    /* !?? problem with ReInit: can't use new method !??
       -> use additional methodIsValid option */
    /*!!! PROBLEM: can't reinit w/o sensitivity if once initialized */
    if ( solver->cvode_mem == NULL )
    {
      solver->cvode_mem = CVodeCreate(method, iteration);
      CVODE_HANDLE_ERROR((void *)(solver->cvode_mem), "CVodeCreate", 0);

      /* !!! max. order should be set here !!! */
      
      /**
       * Call CVodeMalloc to initialize the integrator memory:\n
       *
       * cvode_mem:  pointer to the CVode memory block returned by
       CVodeCreate\n
       * f:     user's right hand side function in f(x,p,t) = dx/dt\n
       * t0:    initial value of time\n
       * y:     the initial dependent variable vector (called in x in the
       *        docu)\n
       * CV_SV: specifies scalar relative and vector absolute tolerances\n
       * reltol: the scalar relative tolerance\n
       * abstol: pointer to the absolute tolerance vector\n
       */
      flag = CVodeMalloc(solver->cvode_mem, rhsFunction,
			 solver->t0, solver->y,
			 CV_SV, solver->reltol, solver->abstol);
      CVODE_HANDLE_ERROR(&flag, "CVodeMalloc", 1);
    }
    else
    {
      flag = CVodeReInit(solver->cvode_mem, rhsFunction,
			 solver->t0, solver->y,
			 CV_SV, solver->reltol, solver->abstol);
      CVODE_HANDLE_ERROR(&flag, "CVodeReInit", 1);
    }

    /**
     * Link the main integrator with data for right-hand side function
     */ 
    flag = CVodeSetFdata(solver->cvode_mem, engine->data);
    CVODE_HANDLE_ERROR(&flag, "CVodeSetFdata", 1);

    /**
     * Link the main integrator with the CVDENSE linear solver
     */
    flag = CVDense(solver->cvode_mem, neq);
    CVODE_HANDLE_ERROR(&flag, "CVDense", 1);

    /**
     * Set the routine used by the CVDENSE linear solver
     * to approximate the Jacobian matrix to ...
     */
    if ( opt->UseJacobian == 1 ) 
      /* ... user-supplied routine Jac */ 
      flag = CVDenseSetJacFn(solver->cvode_mem, jacODE, engine->data);
    else
      /* ...the internal default difference quotient routine CVDenseDQJac */ 
      flag = CVDenseSetJacFn(solver->cvode_mem, NULL, NULL);
    CVODE_HANDLE_ERROR(&flag, "CVDenseSetJacFn", 1);

    /**
     * Set maximum number of internal steps to be taken
     * by the solver in its attempt to reach tout
     */
    flag = CVodeSetMaxNumSteps(solver->cvode_mem, opt->Mxstep);
    CVODE_HANDLE_ERROR(&flag, "CVodeSetMaxNumSteps", 1);

    /**
     * Initialization to compute nonlinear functional
     */
    if ( om->ObjectiveFunction != NULL  )
    {
      if ( engine->solver->nsens != om->nsens && solver->q != NULL ) 
      {
	N_VDestroy_Serial(solver->q);
	CVodeQuadFree(solver->cvode_mem);

        /* Set solver->q to NULL after calling N_VDestroy */
        solver->q = NULL; 
      }

      if ( solver->q == NULL ) /* solver->q has not been initialized  */
      {
	/* although only scalar is neccessary for solver->q, 
           use same dimension as solver->qS (i.e, om->nsens) 
           so that CVodeQuadReInit can be used */
        solver->q = N_VNew_Serial(om->nsens);
	CVODE_HANDLE_ERROR((void *) solver->q,
			   "N_VNew_Serial for vector q", 0);

	/* Init solver->q = 0.0;*/
        for (i=0; i<om->nsens; i++)
	  NV_Ith_S(solver->q, i) = 0.0;

        /* If quadrature memory has not been allocated (in either of
	   CreateCVODE(S)SolverStructures) */ 
        if ( solver->qS == NULL )
	{ 
	  flag = CVodeQuadMalloc(solver->cvode_mem, fQ, solver->q);
	  CVODE_HANDLE_ERROR(&flag, "CVodeQuadMalloc", 1);
	}
        else
	{ 
	  
	  if ( engine->solver->nsens != om->nsens )
	  {
            /* need to do a CVodeQuadFree and CvodeQuadMalloc
             if nsens has changed  */
	    CVodeQuadFree(solver->cvode_mem);
	    flag = CVodeQuadMalloc(solver->cvode_mem, fQ, solver->q);
	    CVODE_HANDLE_ERROR(&flag, "CVodeQuadMalloc", 1);
	  }
	  else
	  {
            /* if nsens has not changed, simply CvodeQuadReInit suffices 
               since a  CVodeQuadMalloc has been called using solver->qS,
               as confirmed by solver->qS != NULL   */
	    flag = CVodeQuadReInit(solver->cvode_mem, fQ, solver->q);
	    CVODE_HANDLE_ERROR(&flag, "CVodeQuadReInit", 1);
	  }
        }      
      }
      else  /* reset solver->q and re-initialize quadrature memory */
      {
	/* Init solver->q = 0.0;*/
        for (i=0; i<om->nsens; i++)
	  NV_Ith_S(solver->q, i) = 0.0;
 
	flag = CVodeQuadReInit(solver->cvode_mem, fQ, solver->q);
	CVODE_HANDLE_ERROR(&flag, "CVodeQuadReInit", 1);
      }

      flag = CVodeSetQuadFdata(solver->cvode_mem, engine);
      CVODE_HANDLE_ERROR(&flag, "CVodeSetQuadFdata", 1);

      /* set quadrature tolerance for objective function 
         to be the same as the forward solution tolerances */
      flag = CVodeSetQuadErrCon(solver->cvode_mem, TRUE,
	CV_SS, solver->reltol, &(opt->Error) );
      CVODE_HANDLE_ERROR(&flag, "CVodeSetQuadErrCon", 1);

    }
     
    if ( opt->Sensitivity )
    {
      flag = IntegratorInstance_createCVODESSolverStructures(engine);
     if ( flag == 0 ) return 0; /* error */ 
    }
    else
    {
      if (solver->yS != NULL)
	CVodeSensToggleOff(solver->cvode_mem);
    } 	


    /* If adjoint is desired, CVadjMalloc needs to be done before
       calling CVodeF  */
    if ( opt->DoAdjoint )
    {
      /*!!! valgrind memcheck adj_sensitivity: 627,528 (280 direct,
	627,248 indirect) bytes in 1 blocks are definitely lost !!!*/
      if ( solver->cvadj_mem == NULL )
      {
	solver->cvadj_mem =
	  CVadjMalloc(solver->cvode_mem, opt->nSaveSteps, CV_HERMITE);
	  CVODE_HANDLE_ERROR((void *)solver->cvadj_mem, "CVadjMalloc", 0);
      }
    }

    /* optimize ODEs for evaluation, only required if no compilation
       was requested */
    if ( !opt->compileFunctions )
      IntegratorInstance_optimizeOdes(engine);
    

  } 
  else
    /* Adjoint Phase*/
  {       
    flag = IntegratorInstance_createCVODESSolverStructures(engine);
    if ( flag == 0 ){
      return 0; /* error */
    }
    /* ERROR HANDLING CODE if SensSolver construction failed */
  }

  engine->isValid = 1; /* 'solver' is consistant with 'data' */

  return 1; /* OK */
}


/* frees N_V vector structures, and the cvode_mem solver */
void IntegratorInstance_freeCVODESolverStructures(integratorInstance_t *engine)
{
  /* Free Forward Sensitivity structures */
  IntegratorInstance_freeForwardSensitivity(engine);

  /* Adjoint related  */
  IntegratorInstance_freeAdjointSensitivity(engine);
    
  /* Free IDA vector dy */
  if (engine->solver->dy != NULL)
  {
    N_VDestroy_Serial(engine->solver->dy);
    engine->solver->dy = NULL;
  }
  
  /* Free the y vector */
  if (engine->solver->y != NULL)
  {
    N_VDestroy_Serial(engine->solver->y);
    engine->solver->y = NULL;
  }

  /* Free forward quadrature vector */
  if (engine->solver->q != NULL)
  {
    N_VDestroy_Serial(engine->solver->q);
    engine->solver->q = NULL;
  }
  
  /* Free the abstol vector */
  if (engine->solver->abstol != NULL)
  {
    N_VDestroy_Serial(engine->solver->abstol);
    engine->solver->abstol = NULL;
  }

  /* Free the integrator memory */
  if (engine->solver->cvode_mem != NULL)
  {
    CVodeFree(&engine->solver->cvode_mem);
    engine->solver->cvode_mem = NULL;
  }

  /* Free the adjoint memory */
  if (engine->solver->cvadj_mem != NULL)
  {
    CVadjFree(&engine->solver->cvadj_mem);
    engine->solver->cvadj_mem = NULL;
  }

}


void IntegratorInstance_freeForwardSensitivity(integratorInstance_t *engine)
{

  /* Free sensitivity vector yS */
  if (engine->solver->yS != NULL)
  {
    N_VDestroyVectorArray_Serial(engine->solver->yS, engine->solver->nsens);
    engine->solver->yS = NULL;
  }

  /* Free sensitivity vector senstol */
  if (engine->solver->senstol != NULL)
  {
    N_VDestroy_Serial(engine->solver->senstol);
    engine->solver->senstol = NULL;
  }

  /* Free sensitivity quadrature vector */
  if (engine->solver->qS != NULL)
  {
    N_VDestroy_Serial(engine->solver->qS);
    engine->solver->qS = NULL;
  }

  CVodeSensFree(engine->solver->cvode_mem);
  
}


void IntegratorInstance_freeAdjointSensitivity(integratorInstance_t *engine)
{
  /* Free adjoint sensitivity vector yA */
  if (engine->solver->yA != NULL)
  {
    N_VDestroy_Serial(engine->solver->yA);
    engine->solver->yA = NULL;
  }

  /* Free adjoint sensitivity quad vector qA */
  if (engine->solver->qA != NULL)
  {
    N_VDestroy_Serial(engine->solver->qA);
    engine->solver->qA = NULL;
  }

  /* Free adjoint sensitivity quad vector abstolA */
  if (engine->solver->abstolA != NULL)
  {
    N_VDestroy_Serial(engine->solver->abstolA);
    engine->solver->abstolA = NULL;
  }

  /* Free adjoint sensitivity quad vector abstolQA */
  if (engine->solver->abstolQA != NULL)
  {
    N_VDestroy_Serial(engine->solver->abstolQA);
    engine->solver->abstolQA = NULL;
  }

}


/** Prints some final statistics of the calls to CVODE routines, that
    are located in CVODE's iopt array.
*/

SBML_ODESOLVER_API int IntegratorInstance_printCVODEStatistics(integratorInstance_t *engine, FILE *f)
{
  int flag;
  long int nst, nfe, nsetups, nje, nni, ncfn, netf;

  cvodeSettings_t *opt = engine->opt;
  cvodeSolver_t *solver = engine->solver;

  flag = CVodeGetNumSteps(solver->cvode_mem, &nst);
  CVODE_HANDLE_ERROR(&flag, "CVodeGetNumSteps", 1);
    
  CVodeGetNumRhsEvals(solver->cvode_mem, &nfe);
  CVODE_HANDLE_ERROR(&flag, "CVodeGetNumRhsEvals", 1);
    
  flag = CVodeGetNumLinSolvSetups(solver->cvode_mem, &nsetups);
  CVODE_HANDLE_ERROR(&flag, "CVodeGetNumLinSolvSetups", 1);
    
  flag = CVDenseGetNumJacEvals(solver->cvode_mem, &nje);
  CVODE_HANDLE_ERROR(&flag, "CVDenseGetNumJacEvals", 1);
    
  flag = CVodeGetNonlinSolvStats(solver->cvode_mem, &nni, &ncfn);
  CVODE_HANDLE_ERROR(&flag, "CVodeGetNonlinSolvStats", 1);
    
  flag = CVodeGetNumErrTestFails(solver->cvode_mem, &netf);
  CVODE_HANDLE_ERROR(&flag, "CVodeGetNumErrTestFails", 1);

  fprintf(f, "\n## Integration Parameters:\n");
  fprintf(f, "## mxstep   = %d rel.err. = %g abs.err. = %g \n",
	  opt->Mxstep, opt->RError, opt->Error);
  fprintf(f, "## CVode Statistics:\n");
  fprintf(f, "## nst = %-6ld nfe  = %-6ld nsetups = %-6ld nje = %ld\n",
	  nst, nfe, nsetups, nje); 
  fprintf(f, "## nni = %-6ld ncfn = %-6ld netf = %ld\n",
	  nni, ncfn, netf);
    
  if ((opt->Sensitivity) | (opt->DoAdjoint))
    return(IntegratorInstance_printCVODESStatistics(engine, f));

  return(1);
}


/*
 * check return values of SUNDIALS functions
 */
int check_flag(void *flagvalue, char *funcname, int opt)
{

  int *errflag;

  /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
  if ( opt == 0 && flagvalue == NULL )
  {
    SolverError_error(FATAL_ERROR_TYPE, SOLVER_ERROR_CVODE_MALLOC_FAILED,
		      "SUNDIALS_ERROR: %s() - returned NULL pointer",
		      funcname);
    return(1);
  }

  /* Check if flag < 0 */
  else if ( opt == 1 )
  {
    errflag = (int *) flagvalue;
    if ( *errflag < 0 )
    {
      SolverError_error(FATAL_ERROR_TYPE, SOLVER_ERROR_CVODE_MALLOC_FAILED,
			"SUNDIALS_ERROR: %s() failed with flag = %d",
			funcname, *errflag);
      return(1);
    }
  }

  /* Check if function returned NULL pointer - no memory allocated */
  else if (opt == 2 && flagvalue == NULL)
  {
    SolverError_error(FATAL_ERROR_TYPE, SOLVER_ERROR_CVODE_MALLOC_FAILED,
		      "SUNDIALS MEMORY_ERROR: %s() failed - returned NULL "
		      "pointer", funcname);
    return(1);
  }

  return(0);
}


/***************** Functions Called by the CVODE Solver ******************/

/**
   f routine: Compute f(t,x) = df/dx .
   
   This function is called by CVODE's integration routines every time
   required. It evaluates the ODEs with the current variable values,
   as supplied by CVODE's N_Vector y vector containing the values of
   all variables (called x in this documentation.  These values are
   first written back to CvodeData.  Then every ODE is passed to
   evaluateAST, together with the cvodeData_t *, and this function
   calculates the current value of the ODE.  The returned value is
   written back to CVODE's N_Vector(ydot) vector that contains the
   values of the ODEs.

*/

static int f(realtype t, N_Vector y, N_Vector ydot, void *f_data)
{
  
  int i;
  realtype *ydata, *dydata;
  cvodeData_t *data;
  data   = (cvodeData_t *) f_data;
  ydata  = NV_DATA_S(y);
  dydata = NV_DATA_S(ydot);

  /* update time  */
  data->currenttime = t;

  /** update parameters: p is modified by CVODES,
      if fS could not be generated  */
  if ( data->opt->Sensitivity && !data->model->sensitivity )
    for ( i=0; i<data->nsens; i++ )
      data->value[data->model->index_sens[i]] = data->p[i];

  /** update ODE variables from CVODE */
  for ( i=0; i<data->model->neq; i++ ) 
    data->value[i] = ydata[i];

  /** update assignment rules */
  for ( i=0; i<data->model->nass; i++ ) 
    if (data->model->assignmentsBeforeODEs[i])
      data->value[data->model->neq+i] =
	evaluateAST(data->model->assignment[i],data);

  /** evaluate ODEs f(x,p,t) = dx/dt */
  for ( i=0; i<data->model->neq; i++ ) 
    dydata[i] = evaluateAST(data->ode[i],data);

  /** reset parameters */
  if ( data->opt->Sensitivity && !data->model->sensitivity )
    for ( i=0; i<data->nsens; i++ )
      data->value[data->model->index_sens[i]] = data->p_orig[i];

  return (0);
}

/**
   Jacobian routine: Compute J(t,x) = df/dx
   
   This function is (optionally) called by CVODE's integration routines
   every time  required.
   Very similar to the f routine, it evaluates the Jacobian matrix
   equations with CVODE's current values and writes the results
   back to CVODE's internal vector DENSE_ELEM(J,i,j).
*/

static int JacODE(long int N, DenseMat J, realtype t,
		  N_Vector y, N_Vector fy, void *jac_data,
		  N_Vector vtemp1, N_Vector vtemp2, N_Vector vtemp3)
{
  
  int i, j;
  realtype *ydata;
  cvodeData_t *data;

  data  = (cvodeData_t *) jac_data;
  ydata = NV_DATA_S(y);
  
  /** update parameters: p is modified by CVODES,
      if fS could not be generated  */
  if ( data->p != NULL && data->opt->Sensitivity )
    for ( i=0; i<data->nsens; i++ )
      data->value[data->model->index_sens[i]] = data->p[i];

  /** update ODE variables from CVODE */
  for ( i=0; i<data->model->neq; i++ ) data->value[i] = ydata[i];

  /** update time */
  data->currenttime = t;

  /** evaluate Jacobian J = df/dx */
  for ( i=0; i<data->model->neq; i++ ) 
    for ( j=0; j<data->model->neq; j++ ) 
      DENSE_ELEM(J,i,j) = evaluateAST(data->model->jacob[i][j], data);

  return (0);
}



static int fQ(realtype t, N_Vector y, N_Vector qdot, void *fQ_data)
{
  int i;
  realtype *ydata, *dqdata;
  cvodeData_t *data;
  cvodeSolver_t *solver; 
  integratorInstance_t *engine;
  
  engine = (integratorInstance_t *) fQ_data;
  solver = engine->solver;
  data  =  engine->data;

  ydata = NV_DATA_S(y);
  dqdata = NV_DATA_S(qdot);

  /* update ODE variables from CVODE  */  
  for ( i=0; i<data->model->neq; i++ ) data->value[i] = ydata[i];
 
  /* update time */
  data->currenttime = t;

  /* evaluate quadrature integrand */
  for ( i=0; i<data->model->nsens; i++ )
    dqdata[i] = 0.0;

  /* only the first component matters */
  dqdata[0] += evaluateAST(engine->om->ObjectiveFunction, data);

  return (0);
}



/*! @} */
/* End of file */
