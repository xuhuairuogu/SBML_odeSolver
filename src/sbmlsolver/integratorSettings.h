/*
  Last changed Time-stamp: <2006-09-28 17:08:39 raim>
  $Id: integratorSettings.h,v 1.25 2006/09/28 18:14:28 raimc Exp $ 
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

 *     
 */
#include "sbmlsolver/exportdefs.h"

#ifndef _CVODESETTINGS_H_
#define _CVODESETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

  /* structures */
  typedef struct timeSettings timeSettings_t;
  typedef struct cvodeSettings cvodeSettings_t;
  
  /** Settings for CVODE Integration

      This structure contains the basic settings for an integration run,
      like the time course, relative and absolute erros,
      integration methods etc.
      
      It also contains diverse switches for integrator behaviour, as well
      as for setting sensitivity analysis
  */
  struct cvodeSettings {
    double Time;          /**< End time to which model is integrated or 
			     step size if 'Indefinitely' is true */
    int PrintStep;        /**< Number of output steps from 0 to 'Time';
			     ignored if 'Indefinitely' */
    double *TimePoints;   /**< Optional array for designed time-courses.
			     If passed by the calling application,
			     Time will be ignored and overruled by
			     TimePoints[Printstep+1], otherwise TimePoints
			     will be calculated from Time and PrintSteps */
    int Indefinitely;     /**< if not 0: run without a defined end
			     time, Time field contains step
			     duration, ignore PrintStep field*/
    double Error;         /**< absolute tolerance in Cvode integration */
    double RError;        /**< relative tolerance in Cvode integration */
    int Mxstep;           /**< maximum step number for CVode integration */
    int CvodeMethod;      /**< set ADAMS-MOULTON (1) or BDF (0)
			     nonlinear solver */
    int IterMethod;       /**< set type of nonlinear solver iteration
			     Newton (0) or Functional (1) */
    int MaxOrder;         /**< set maximum order of ADAMS or BDF method */
    int ResetCvodeOnEvent; /**< restart CVODE when event is triggered */
    int Sensitivity;      /**< if not 0: use CVODES for sensitivity analysis */
    char **sensIDs;       /**< ID's for parameters and initial conditions 
			     for sensitivity analysis */ 
    int nsens;
    int SensMethod;       /**< set sensitivity analysis method:
			     0: SIMULTANEOUS,
			     1: STAGGERED,
			     2: STAGGERED1   */    
    int HaltOnEvent;      /**< if not 0: Stop integration upon an event */
    int SteadyState;      /**< if not 0: Stop integration upon a
			     steady state */
    int UseJacobian;      /**< use of Jacobian ASTs (1) or CVODES'
			     internal approximation (0)*/
    int StoreResults;     /**< if not 0: store time course history */

    int compileFunctions ;  /**< if 1 use compiled functions for ODE,
			       Jacobian and events */

    /** Adjoint related flags and settings   */   
    int DoAdjoint;          /**< if 1, the adjoint solution is desired   */
    int AdjointPhase;       /**< if 0, do the forward phase of the normal run 
			       or the forward phase in preparation for
			       the adjoint  */
    
    double AdjTime;          /**< Time to which model is integrated or if
				step size if 'Indefinitely' is true */
    int AdjPrintStep;        /**< Number of output steps from 0 to 'Time';
				ignored if 'Indefinitely' */ 
     
    double *AdjTimePoints;   /**< Optional array of designed time-course.
				If passed by the calling application,
				AdjTime will be ignored and overruled by
				AdjTimePoints[AdjPrintstep+1], otherwise
				AdjTimePoints will be calculated from
				AdjTime and AdjPrintSteps */

    int nSaveSteps;           /**< Number of steps saved in forward phase  */
    int ncheck;              /**< Number of checkpoints, as returned by
				CvodeF */

    double AdjError;         /**< absolute tolerance in adjoint integration */
    double AdjRError;        /**< relative tolerance in adjoint integration */ 
    int AdjStoreResults;     /**< if not 0: Store adjoint time
				course history */


  } ;

  /** NOT USED CURRENTLY!
      timeSettings: start- and end times, timesteps, number of timesteps */
  struct timeSettings {
    double t0;
    double tmult;
    double tend;
    int nout;
  } ;

  /* functions */
  /* ??might be implented?? */
  timeSettings_t *TimeSettings_create(double t0, double tend, int nout);
  void TimeSettings_free(timeSettings_t *time);
  cvodeSettings_t *CvodeSettings_createFromTimeSettings(timeSettings_t *time);

  /* create and free, get and set cvodeSettings */
  SBML_ODESOLVER_API cvodeSettings_t *CvodeSettings_create();
  SBML_ODESOLVER_API cvodeSettings_t *CvodeSettings_createWithTime(double Time, int PrintStep);
  SBML_ODESOLVER_API cvodeSettings_t *CvodeSettings_createWith(double EndTime, int PrintStep, double Error, double RError, int Mxstep, int Method, int IterMethod, int UseJacobian, int Indefinitely, int HaltOnEvent, int SteadyState, int StoreResults, int Sensitivity, int SensMethod);
  SBML_ODESOLVER_API int CvodeSettings_setTime(cvodeSettings_t *, double EndTime, int PrintStep);
  SBML_ODESOLVER_API int CvodeSettings_setTimeStep(cvodeSettings_t *, int, double);
  SBML_ODESOLVER_API int CvodeSettings_setTimeSeries(cvodeSettings_t *, double *timeseries, int PrintStep);
  SBML_ODESOLVER_API void CvodeSettings_setSwitches(cvodeSettings_t *, int UseJacobian, int Indefinitely, int HaltOnEvent, int SteadyState, int StoreResults, int Sensitivity, int SensMethod);
  SBML_ODESOLVER_API void CvodeSettings_setErrors(cvodeSettings_t *, double Error, double RError, int Mxstep);
  SBML_ODESOLVER_API void CvodeSettings_setError(cvodeSettings_t *, double);
  SBML_ODESOLVER_API void CvodeSettings_setRError(cvodeSettings_t *, double);
  SBML_ODESOLVER_API void CvodeSettings_setMxstep(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setCompileFunctions(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setResetCvodeOnEvent(cvodeSettings_t *, int);

  /* Adjoint setttings */
  SBML_ODESOLVER_API void CvodeSettings_setDoAdj(cvodeSettings_t *);
  SBML_ODESOLVER_API void CvodeSettings_unsetDoAdj(cvodeSettings_t *);
  SBML_ODESOLVER_API void CvodeSettings_setAdjPhase(cvodeSettings_t *); 
  SBML_ODESOLVER_API void CvodeSettings_unsetAdjPhase(cvodeSettings_t *); 
  SBML_ODESOLVER_API void CvodeSettings_setAdjErrors(cvodeSettings_t *, double Error, double RError);
  SBML_ODESOLVER_API void CvodeSettings_setAdjError(cvodeSettings_t *, double);
  SBML_ODESOLVER_API void CvodeSettings_setAdjRError(cvodeSettings_t *, double);
  SBML_ODESOLVER_API void CvodeSettings_setnSaveSteps(cvodeSettings_t *, int);
  SBML_ODESOLVER_API int CvodeSettings_setAdjTime(cvodeSettings_t *, double EndTime, int PrintStep);
  SBML_ODESOLVER_API  int CvodeSettings_setAdjTimeSeries(cvodeSettings_t *set, double *timeseries, int AdjPrintStep, double EndTime);


  SBML_ODESOLVER_API void CvodeSettings_setMethod(cvodeSettings_t *, int, int);
  SBML_ODESOLVER_API void CvodeSettings_setIterMethod(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setMaxOrder(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setJacobian(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setIndefinitely(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setHaltOnEvent(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setSteadyState(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setStoreResults(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_setSensitivity(cvodeSettings_t *, int);
  SBML_ODESOLVER_API int CvodeSettings_setSensParams(cvodeSettings_t *, char **, int);
  SBML_ODESOLVER_API void CvodeSettings_setSensMethod(cvodeSettings_t *, int);
  SBML_ODESOLVER_API void CvodeSettings_dump(cvodeSettings_t *);
  SBML_ODESOLVER_API void CvodeSettings_free(cvodeSettings_t *);
  SBML_ODESOLVER_API cvodeSettings_t *CvodeSettings_clone(cvodeSettings_t *);
  
  SBML_ODESOLVER_API double CvodeSettings_getEndTime(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getPrintsteps(cvodeSettings_t *);
  SBML_ODESOLVER_API double CvodeSettings_getTimeStep(cvodeSettings_t *);
  SBML_ODESOLVER_API double CvodeSettings_getTime(cvodeSettings_t *, int);  
  SBML_ODESOLVER_API double CvodeSettings_getError(cvodeSettings_t *);
  SBML_ODESOLVER_API double CvodeSettings_getRError(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getMxstep(cvodeSettings_t *);
  SBML_ODESOLVER_API char *CvodeSettings_getMethod(cvodeSettings_t *);
  SBML_ODESOLVER_API char *CvodeSettings_getIterMethod(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getMaxOrder(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getCompileFunctions(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getResetCvodeOnEvent(cvodeSettings_t *);

  SBML_ODESOLVER_API int CvodeSettings_getJacobian(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getIndefinitely(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getHaltOnEvent(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getSteadyState(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getStoreResults(cvodeSettings_t *);
  SBML_ODESOLVER_API int CvodeSettings_getSensitivity(cvodeSettings_t *);
  SBML_ODESOLVER_API char *CvodeSettings_getSensMethod(cvodeSettings_t *);

  
#ifdef __cplusplus
}
#endif

#endif /* _CVODESETTINGS_H_ */

