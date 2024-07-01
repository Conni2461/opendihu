#include "data_management/specialized_solver/dynamic_hyperelasticity_solver.h"

namespace Data {

template <typename FunctionSpaceType>
DynamicHyperelasticitySolver<FunctionSpaceType>::DynamicHyperelasticitySolver(
    DihuContext context)
    : Data<FunctionSpaceType>::Data(context) {}

template <typename FunctionSpaceType>
bool DynamicHyperelasticitySolver<FunctionSpaceType>::restoreState(
    const InputReader::Generic &r) {
  std::vector<double> displacements, velocities, internalVirtualWork,
      accelerationTerm, externalVirtualWorkDead;
  if (!r.readDoubleVector(this->displacements_->name().c_str(),
                          displacements)) {
    return false;
  }
  if (!r.readDoubleVector(this->velocities_->name().c_str(), velocities)) {
    return false;
  }
  if (!r.readDoubleVector(this->internalVirtualWork_->name().c_str(),
                          internalVirtualWork)) {
    return false;
  }
  if (!r.readDoubleVector(this->accelerationTerm_->name().c_str(),
                          accelerationTerm)) {
    return false;
  }
  if (!r.readDoubleVector(this->externalVirtualWorkDead_->name().c_str(),
                          externalVirtualWorkDead)) {
    return false;
  }

  displacements_->setValues(displacements);
  velocities_->setValues(velocities);
  internalVirtualWork_->setValues(internalVirtualWork);
  accelerationTerm_->setValues(accelerationTerm);
  externalVirtualWorkDead_->setValues(externalVirtualWorkDead);

  // TODO(conni2461): restore geometry_
  return true;
}

template <typename FunctionSpaceType>
void DynamicHyperelasticitySolver<FunctionSpaceType>::createPetscObjects() {
  LOG(DEBUG) << "DynamicHyperelasticitySolver::createPetscObjects";

  assert(this->functionSpace_);

  std::vector<std::string> displacementsComponentNames({"x", "y", "z"});
  displacements_ = this->functionSpace_->template createFieldVariable<3>(
      "u", displacementsComponentNames);
  velocities_ = this->functionSpace_->template createFieldVariable<3>(
      "v", displacementsComponentNames);
  internalVirtualWork_ = this->functionSpace_->template createFieldVariable<3>(
      "δWint_displacement", displacementsComponentNames);
  accelerationTerm_ = this->functionSpace_->template createFieldVariable<3>(
      "δWint_acceleration", displacementsComponentNames);
  externalVirtualWorkDead_ =
      this->functionSpace_->template createFieldVariable<3>(
          "δWext", displacementsComponentNames);
}

//! field variable of u
template <typename FunctionSpaceType>
std::shared_ptr<typename DynamicHyperelasticitySolver<
    FunctionSpaceType>::DisplacementsFieldVariableType>
DynamicHyperelasticitySolver<FunctionSpaceType>::displacements() {
  return this->displacements_;
}

//! field variable of v
template <typename FunctionSpaceType>
std::shared_ptr<typename DynamicHyperelasticitySolver<
    FunctionSpaceType>::DisplacementsFieldVariableType>
DynamicHyperelasticitySolver<FunctionSpaceType>::velocities() {
  return this->velocities_;
}

//! field variable of u_compressible
template <typename FunctionSpaceType>
std::shared_ptr<typename DynamicHyperelasticitySolver<
    FunctionSpaceType>::DisplacementsFieldVariableType>
DynamicHyperelasticitySolver<FunctionSpaceType>::externalVirtualWorkDead() {
  return this->externalVirtualWorkDead_;
}

//! field variable of ∂W_int_compressible
template <typename FunctionSpaceType>
std::shared_ptr<typename DynamicHyperelasticitySolver<
    FunctionSpaceType>::DisplacementsFieldVariableType>
DynamicHyperelasticitySolver<FunctionSpaceType>::internalVirtualWork() {
  return this->internalVirtualWork_;
}

//! field variable of
template <typename FunctionSpaceType>
std::shared_ptr<typename DynamicHyperelasticitySolver<
    FunctionSpaceType>::DisplacementsFieldVariableType>
DynamicHyperelasticitySolver<FunctionSpaceType>::accelerationTerm() {
  return this->accelerationTerm_;
}

template <typename FunctionSpaceType>
typename DynamicHyperelasticitySolver<
    FunctionSpaceType>::FieldVariablesForOutputWriter
DynamicHyperelasticitySolver<
    FunctionSpaceType>::getFieldVariablesForOutputWriter() {
  return std::make_tuple(
      std::shared_ptr<DisplacementsFieldVariableType>(
          std::make_shared<typename FunctionSpaceType::GeometryFieldType>(
              this->functionSpace_->geometryField())), // geometry
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->displacements_), // displacements_
      std::shared_ptr<DisplacementsFieldVariableType>(this->velocities_),
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->internalVirtualWork_),
      std::shared_ptr<DisplacementsFieldVariableType>(this->accelerationTerm_),
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->externalVirtualWorkDead_));
}

template <typename FunctionSpaceType>
typename DynamicHyperelasticitySolver<
    FunctionSpaceType>::FieldVariablesForCheckpointing
DynamicHyperelasticitySolver<
    FunctionSpaceType>::getFieldVariablesForCheckpointing() {
  return this->getFieldVariablesForOutputWriter();
}
} // namespace Data
