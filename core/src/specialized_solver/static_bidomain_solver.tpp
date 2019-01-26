#include "specialized_solver/static_bidomain_solver.h"

#include <Python.h>  // has to be the first included header

#include "utility/python_utility.h"
#include "utility/petsc_utility.h"
#include "data_management/specialized_solver/multidomain.h"

namespace TimeSteppingScheme
{

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
StaticBidomainSolver(DihuContext context) :
  context_(context["StaticBidomainSolver"]),
  data_(this->context_), finiteElementMethodPotentialFlow_(this->context_["PotentialFlow"]),
  finiteElementMethodDiffusionTransmembrane_(this->context_["Activation"]), finiteElementMethodDiffusionExtracellular_(this->context_["Activation"]),
  rankSubset_(std::make_shared<Partition::RankSubset>()), initialized_(false)
{
  // get python config
  this->specificSettings_ = this->context_.getPythonConfig();

  // read in the durationLogKey
  if (specificSettings_.hasKey("durationLogKey"))
  {
    this->durationLogKey_ = specificSettings_.getOptionString("durationLogKey", "");
  }

  // initialize output writers
  this->outputWriterManager_.initialize(this->context_, this->specificSettings_);
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
advanceTimeSpan()
{
  // start duration measurement, the name of the output variable can be set by "durationLogKey" in the config
  if (this->durationLogKey_ != "")
    Control::PerformanceMeasurement::start(this->durationLogKey_);

  //this->solveLinearSystem();

  // stop duration measurement
  if (this->durationLogKey_ != "")
    Control::PerformanceMeasurement::stop(this->durationLogKey_);

  // write current output values
  this->outputWriterManager_.writeOutput(this->data_, 0, endTime_);
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
run()
{
  // initialize everything
  initialize();

  this->advanceTimeSpan();
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
setTimeSpan(double startTime, double endTime)
{
  endTime_ = endTime;
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
initialize()
{
  if (this->initialized_)
    return;

  LOG(DEBUG) << "initialize static_bidomain_solver";
  assert(this->specificSettings_.pyObject());

  // initialize the potential flow finite element method, this also creates the function space
  finiteElementMethodPotentialFlow_.initialize();

  // initialize the data object
  data_.setFunctionSpace(finiteElementMethodPotentialFlow_.functionSpace());
  data_.initialize();

  LOG(INFO) << "Run potential flow simulation for fiber directions.";

  // solve potential flow Laplace problem
  finiteElementMethodPotentialFlow_.run();

  LOG(DEBUG) << "compute gradient field";

  // compute a gradient field from the solution of the potential flow
  data_.flowPotential()->setValues(*finiteElementMethodPotentialFlow_.data().solution());
  data_.flowPotential()->computeGradientField(data_.fiberDirection());

  LOG(DEBUG) << "flow potential: " << *data_.flowPotential();
  LOG(DEBUG) << "fiber direction: " << *data_.fiberDirection();

  // initialize the finite element class, from which only the stiffness matrix is needed
  // diffusion object without prefactor, for normal diffusion (2nd multidomain eq.)
  finiteElementMethodDiffusionTransmembrane_.initialize(data_.fiberDirection(), nullptr);
  // direction, spatiallyVaryingPrefactor, useAdditionalDiffusionTensor=false
  finiteElementMethodDiffusionExtracellular_.initialize(data_.fiberDirection(), nullptr, true);

  // initialize system matrix
  setSystemMatrix();

  LOG(DEBUG) << "initialize linear solver";

  // initialize linear solver
  if (linearSolver_ == nullptr)
  {
    // retrieve linear solver
    linearSolver_ = this->context_.solverManager()->template solver<Solver::Linear>(
      this->specificSettings_, this->rankSubset_->mpiCommunicator());
  }

  LOG(DEBUG) << "set system matrix to linear solver";

  // set matrix used for linear solver and preconditioner to ksp context
  assert(this->linearSolver_->ksp());
  PetscErrorCode ierr;
  ierr = KSPSetOperators(*this->linearSolver_->ksp(), systemMatrix_, systemMatrix_); CHKERRV(ierr);

  // initialize rhs and solution vector
  subvectorsRightHandSide_.resize(2);
  subvectorsSolution_.resize(2);

  // set values for Vm
  subvectorsRightHandSide_[0] = data_.transmembranePotential()->valuesGlobal();
  subvectorsSolution_[0] = data_.transmembranePotentialSolution()->valuesGlobal();

  // set values for phi_e
  subvectorsRightHandSide_[1] = data_.zero()->valuesGlobal();
  subvectorsSolution_[1] = data_.extraCellularPotential()->valuesGlobal();
  ierr = VecZeroEntries(subvectorsSolution_[1]); CHKERRV(ierr);

  // create the nested vectors
  LOG(DEBUG) << "create nested vector";
  ierr = VecCreateNest(MPI_COMM_WORLD, 2, NULL, subvectorsRightHandSide_.data(), &rightHandSide_); CHKERRV(ierr);
  ierr = VecCreateNest(MPI_COMM_WORLD, 2, NULL, subvectorsSolution_.data(), &solution_); CHKERRV(ierr);

  LOG(DEBUG) << "initialization done";
  this->initialized_ = true;
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::reset()
{
  this->initialized_ = false;
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
setSystemMatrix()
{
  std::vector<Mat> submatrices(4, NULL);

  LOG(TRACE) << "setSystemMatrix";

  // fill submatrices, empty submatrices may be NULL
  Mat stiffnessMatrixTopLeft = finiteElementMethodDiffusionTransmembrane_.data().stiffnessMatrix()->valuesGlobal();

  // set bottom right matrix
  Mat stiffnessMatrixBottomRight = finiteElementMethodDiffusionExtracellular_.data().stiffnessMatrix()->valuesGlobal();

  submatrices[0] = stiffnessMatrixTopLeft;
  submatrices[3] = stiffnessMatrixBottomRight;

  // create nested matrix
  PetscErrorCode ierr;
  ierr = MatCreateNest(this->data_.functionSpace()->meshPartition()->mpiCommunicator(),
                       2, NULL, 2, NULL, submatrices.data(), &this->systemMatrix_); CHKERRV(ierr);
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
solveLinearSystem()
{
  PetscErrorCode ierr;

  VLOG(1) << "in solveLinearSystem";

  // configure that the initial value for the iterative solver is the value in solution, not zero
  ierr = KSPSetInitialGuessNonzero(*this->linearSolver_->ksp(), PETSC_TRUE); CHKERRV(ierr);

  // solve the system, KSPSolve(ksp,b,x)
  ierr = KSPSolve(*this->linearSolver_->ksp(), rightHandSide_, solution_); CHKERRV(ierr);

  int numberOfIterations = 0;
  PetscReal residualNorm = 0.0;
  ierr = KSPGetIterationNumber(*this->linearSolver_->ksp(), &numberOfIterations); CHKERRV(ierr);
  ierr = KSPGetResidualNorm(*this->linearSolver_->ksp(), &residualNorm); CHKERRV(ierr);

  KSPConvergedReason convergedReason;
  ierr = KSPGetConvergedReason(*this->linearSolver_->ksp(), &convergedReason); CHKERRV(ierr);

  lastNumberOfIterations_ = numberOfIterations;

  LOG(DEBUG) << "Linear system of bidomain problem solved in " << numberOfIterations << " iterations, residual norm " << residualNorm
    << ": " << PetscUtility::getStringLinearConvergedReason(convergedReason);
}

//! return whether the underlying discretizableInTime object has a specified mesh type and is not independent of the mesh type
template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
bool StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
knowsMeshType()
{
  return true;
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
typename StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::Data &StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
data()
{
  return data_;
}

//! get the data that will be transferred in the operator splitting to the other term of the splitting
//! the transfer is done by the solution_vector_mapping class
template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
typename StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::TransferableSolutionDataType
StaticBidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
getSolutionForTransfer()
{
  return this->data_.transmembranePotential();
}

} // namespace TimeSteppingScheme
