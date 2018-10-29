#pragma once

#include <Python.h>  // has to be the first included header

#include <Python.h>
#include <vector>
#include <list>

#include "control/dihu_context.h"
#include "output_writer/manager.h"
#include "cellml/00_cellml_adapter_base.h"

/** This is a class that handles the right hand side routine of CellML
 * It needs access on the system size and some C-code of the right hand side.
 * It provides a method to call the right hand side.
 *
 *  Naming:
 *   Intermediate (opendihu) = KNOWN (OpenCMISS) = Algebraic (OpenCOR)
 *   Parameter (opendihu, OpenCMISS) = KNOWN (OpenCMISS), in OpenCOR also algebraic
 *   Constant - these are constants that are only present in the source files
 *   State: state variable
 *   Rate: the time derivative of the state variable, i.e. the increment value in an explicit Euler stepping
 */
template <int nStates, typename FunctionSpaceType>
class RhsRoutineHandler:
  public CellmlAdapterBase<nStates,FunctionSpaceType>
{
public:

  //! constructor
  using CellmlAdapterBase<nStates,FunctionSpaceType>::CellmlAdapterBase;

protected:
  //! given a normal cellml source file for rhs routine create a second file for multiple instances. @return: if successful
  bool createSimdSourceFile(std::string &simdSourceFilename);

  //! given a normal cellml source file for rhs routine create a third file for gpu acceloration. @return: if successful
  bool createGPUSourceFile(std::string &gpuSourceFilename);

  //! initialize the rhs routine, either directly from a library or compile it
  void initializeRhsRoutine();

  //! load the library (<file>.so) that was created earlier, store
  bool loadRhsLibrary(std::string libraryFilename);

  //! scan the given cellml source file for initial values that are given by dummy assignments (OpenCMISS) or directly (OpenCOR). This also sets nParameters_, nConstants_ and nIntermediates_
  bool scanSourceFile(std::string sourceFilename, std::array<double,nStates> &statesInitialValues);

  bool useGivenLibrary_;   ///< if the given library at libraryFileName_ should be loaded instead of compiling on our own

  std::vector<std::string> constantAssignments_;   ///< source code lines where constant variables are assigned

  void (*rhsRoutine_)(void *context, double t, double *states, double *rates, double *algebraics, double *parameters);  ///< function pointer to the rhs routine that can compute several instances of the problem in parallel. Data is assumed to contain values for a state contiguously, e.g. (state[1], state[1], state[1], state[2], state[2], state[2], ...). The first parameter is a this pointer.

  //! helper rhs routines
  void (*rhsRoutineOpenCMISS_)(double VOI, double* OC_STATE, double* OC_RATE, double* OC_WANTED, double* OC_KNOWN);   ///< function pointer to a rhs function that is passed as dynamic library, computes rates and intermediate values from states. The parameters are: VOI, STATES, RATES, WANTED, KNOWN, (VOI: current simulation time, not used)
  void (*rhsRoutineGPU_)(void *context, double t, double *states, double *rates, double *algebraics, double *parameters);   ///< function pointer to a gpu processed rhs function that is passed as dynamic library. Data is assumed to contain values for a state contiguously, e.g. (state[1], state[1], state[1], state[2], state[2], state[2], ...).
  void (*initConstsOpenCOR_)(double* CONSTANTS, double* RATES, double *STATES); ///< function pointer to a function that initializes everything, generated by OpenCOR
  void (*computeRatesOpenCOR_)(double VOI, double* CONSTANTS, double* RATES, double* STATES, double* ALGEBRAIC); ///< function pointer to a function that computes the rates from the current states, generated by OpenCOR
  void (*computeVariablesOpenCOR_)(double VOI, double* CONSTANTS, double* RATES, double* STATES, double* ALGEBRAIC); ///< function pointer to a function that computes the algebraic variables from the current states, generated by OpenCOR
};

#include "cellml/01_rhs_routine_handler.tpp"
