/*
  Last changed Time-stamp: <2008-09-19 15:05:31 raim>
  $Id: FIMtest.c,v 1.3 2009/02/11 15:07:16 stefan_tbi Exp $
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
 *     Stefan Mueller and James Lu
 *
 * Contributor(s):
 */

#include <stdio.h>
#include <stdlib.h>

#include <sbmlsolver/odeSolver.h>
#include <sbmlsolver/sensSolver.h>


int
main (int argc, char *argv[])
{
  int i, j, k;
  
  odeModel_t *om;
  cvodeSettings_t *set;
  integratorInstance_t *ii;

  variableIndex_t *p1, *p2, *p3;
  double *weights;
  
  /* Setting SBML ODE Solver integration parameters  */
  set = CvodeSettings_create();
  CvodeSettings_setTime(set, 30, 10);
  CvodeSettings_setErrors(set, 1e-15, 1e-10, 1e9);
  CvodeSettings_setMethod(set, 0, 5);
  /*   CvodeSettings_setStoreResults(set, 0); */
  CvodeSettings_setJacobian(set, 1);         /* for testing only */
  CvodeSettings_setCompileFunctions(set, 0); /* for testing only */
  /* CvodeSettings_dump(set); */
  CvodeSettings_setFIM(set);                 /* ACTIVATE FIM */
  
  /* creating the odeModel */
  om = ODEModel_createFromFile("MAPK.xml");
  ii = IntegratorInstance_create(om, set);

  printf("\nFirst try a few integrations with default sensitivity\n");
  IntegratorInstance_dumpNames(ii);
  printf("\n");
  for ( i=0; i<2; i++ )
  {
    printf("Run #%d:\n", i);
    IntegratorInstance_integrate(ii);
/*     IntegratorInstance_dumpData(ii); */
    IntegratorInstance_printResults(ii, stderr);
    IntegratorInstance_reset(ii);
    printf("finished\n\n");
  }

  /* ACTIVATE SENSITIVITY ANALYSIS */

  CvodeSettings_setSensitivity(set, 1);
  /* 0: simultaneous 1: staggered, 2: staggered1
     see CVODES user guide for details */
  CvodeSettings_setSensMethod(set, 0);

  /* reset integrator to new settings */
  IntegratorInstance_reset(ii);

  printf("Now Activate Sensitivity\n\n");
  printf("ii->os->nsens   = %i\n\n", ii->data->nsens);
  for ( i=0; i<ii->data->nsens; i++ )  
      printf("%s\n", ODEModel_getVariableName(ii->om, ODESense_getSensParamIndexByNum(ii->os, i)) );

  printf("\n");
  printf("sensitivities calculated for all constants\n");
  
  p1 = ODESense_getSensParamIndexByNum(ii->os, 1);
  p2 = ODESense_getSensParamIndexByNum(ii->os, 2);
  p3 = ODESense_getSensParamIndexByNum(ii->os, 3);
  printf("sensitivities printed for constants %s, %s, %s\n\n",
	 ODEModel_getVariableName(ii->om, p1),
	 ODEModel_getVariableName(ii->om, p2),
	 ODEModel_getVariableName(ii->om, p3));

  /* create non-default weights for computation of FIM */
  /* weights should be extracted from objective function! */
  ASSIGN_NEW_MEMORY_BLOCK(weights, ii->data->neq, double, 0);
  for ( i=0; i<ii->data->neq; i++ )
      weights[i] = 1.;
  /* set weights (to non-default values) */
  IntegratorInstance_setFIMweights(ii, weights, ii->data->neq);
    
  /* *** *** *** *** *** *** discrete data *** *** *** *** *** *** *** */
  CvodeSettings_setDiscreteObservation(set);
  printf("DISCRETE DATA\n\n");

  i = 0;
  while ( i < 2 )
  {
    printf("Run #%d:\n", i);
    while( !IntegratorInstance_timeCourseCompleted(ii) )
    {
      IntegratorInstance_dumpPSensitivities(ii, p1);
      IntegratorInstance_dumpPSensitivities(ii, p2);
      IntegratorInstance_dumpPSensitivities(ii, p3);
      if ( !IntegratorInstance_integrateOneStep(ii) )
	break;
    }    
    IntegratorInstance_dumpPSensitivities(ii, p1);
    IntegratorInstance_dumpPSensitivities(ii, p2);
    IntegratorInstance_dumpPSensitivities(ii, p3);

    fprintf(stderr, "FIM =\n");
    for ( j=0; j<ii->data->nsens; j++ )
    {
      for ( k=0; k<ii->data->nsens; k++ )
	  fprintf(stderr, "%g ", ii->data->FIM[j][k]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

    IntegratorInstance_reset(ii);
    
    i++;
  }

  /* *** *** *** *** *** *** continuous data *** *** *** *** *** *** *** */
  CvodeSettings_unsetDiscreteObservation(set);
  printf("CONTINUOUS DATA\n\n");

  i = 0;
  while ( i < 2 )
  {
    printf("Run #%d:\n", i);
    while( !IntegratorInstance_timeCourseCompleted(ii) )
    {
      IntegratorInstance_dumpPSensitivities(ii, p1);
      IntegratorInstance_dumpPSensitivities(ii, p2);
      IntegratorInstance_dumpPSensitivities(ii, p3);
      if ( !IntegratorInstance_integrateOneStep(ii) )
	break;
    }    
    IntegratorInstance_dumpPSensitivities(ii, p1);
    IntegratorInstance_dumpPSensitivities(ii, p2);
    IntegratorInstance_dumpPSensitivities(ii, p3);

    /* calculate FIM */
    IntegratorInstance_CVODEQuad(ii);

    fprintf(stderr, "FIM =\n");
    for ( j=0; j<ii->data->nsens; j++ )
    {
      for ( k=0; k<ii->data->nsens; k++ )
	  fprintf(stderr, "%g ", ii->data->FIM[j][k]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");

    IntegratorInstance_reset(ii);
    
    i++;
  }

  fprintf(stderr, "finished\n");

  /*   VariableIndex_free(y); */
  VariableIndex_free(p1);
  VariableIndex_free(p2);
  free(weights);
  
  /* now we have the results and can free the inputs */
  IntegratorInstance_free(ii);
  CvodeSettings_free(set);
  ODEModel_free(om);

  return (EXIT_SUCCESS);  
}

/* End of file */
