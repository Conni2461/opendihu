#include "data_management/specialized_solver/quasi_static_nonlinear_elasticity_febio.h"

#include "easylogging++.h"

#include "utility/python_utility.h"
#include "control/dihu_context.h"
#include "utility/petsc_utility.h"

namespace Data {

QuasiStaticNonlinearElasticityFebio::QuasiStaticNonlinearElasticityFebio(
    DihuContext context)
    : Data<FunctionSpace>::Data(context) {}

void QuasiStaticNonlinearElasticityFebio::initialize() {
  // call initialize of base class
  Data<FunctionSpace>::initialize();

  slotConnectorData_ = std::make_shared<SlotConnectorDataType>();
  slotConnectorData_->addFieldVariable(activation());

  // parse slot names for all slot connector data slots, only one slot here
  this->context_.getPythonConfig().getOptionVector(
      "slotNames", slotConnectorData_->slotNames);

  // make sure that there are as many slot names as slots
  slotConnectorData_->slotNames.resize(slotConnectorData_->nSlots());
}

bool QuasiStaticNonlinearElasticityFebio::restoreState(
    const InputReader::HDF5 &r) {
  std::vector<double> activation, displacements, reactionForce, cauchyStress,
      pk2Stress, greenLagrangeStrain, relativeVolume;
  if (!r.readDoubleVector(this->activation_->name().c_str(), activation)) {
    return false;
  }
  if (!r.readDoubleVector(this->displacements_->name().c_str(),
                          displacements)) {
    return false;
  }
  if (!r.readDoubleVector(this->reactionForce_->name().c_str(),
                          reactionForce)) {
    return false;
  }
  if (!r.readDoubleVector(this->cauchyStress_->name().c_str(), cauchyStress)) {
    return false;
  }
  if (!r.readDoubleVector(this->pk2Stress_->name().c_str(), pk2Stress)) {
    return false;
  }
  if (!r.readDoubleVector(this->greenLagrangeStrain_->name().c_str(),
                          greenLagrangeStrain)) {
    return false;
  }
  if (!r.readDoubleVector(this->relativeVolume_->name().c_str(),
                          relativeVolume)) {
    return false;
  }

  this->activation_->setValues(activation);
  this->displacements_->setValues(displacements);
  this->reactionForce_->setValues(reactionForce);
  this->cauchyStress_->setValues(cauchyStress);
  this->pk2Stress_->setValues(pk2Stress);
  this->greenLagrangeStrain_->setValues(greenLagrangeStrain);
  this->relativeVolume_->setValues(relativeVolume);

  // TODO(conni2461): restore geometry
  this->referenceGeometry_ = std::make_shared<FieldVariableTypeVector>(
      this->functionSpace_->geometryField(), "referenceGeometry");
  return true;
}

void QuasiStaticNonlinearElasticityFebio::createPetscObjects() {
  LOG(DEBUG) << "QuasiStaticNonlinearElasticityFebio::createPetscObjects";

  assert(this->functionSpace_);

  activation_ =
      this->functionSpace_->template createFieldVariable<1>("activation");
  displacements_ = this->functionSpace_->template createFieldVariable<3>("u");
  reactionForce_ =
      this->functionSpace_->template createFieldVariable<3>("reactionForce");
  cauchyStress_ = this->functionSpace_->template createFieldVariable<6>(
      "sigma (Cauchy stress)");
  pk2Stress_ =
      this->functionSpace_->template createFieldVariable<6>("S (Pk2 stress)");
  greenLagrangeStrain_ = this->functionSpace_->template createFieldVariable<6>(
      "E (Green-Lagrange strain)");
  relativeVolume_ = this->functionSpace_->template createFieldVariable<1>("J");

  // copy initial geometry to referenceGeometry
  referenceGeometry_ = std::make_shared<FieldVariableTypeVector>(
      this->functionSpace_->geometryField(), "referenceGeometry");

  LOG(DEBUG) << "pointer referenceGeometry: "
             << referenceGeometry_->partitionedPetscVec();
  LOG(DEBUG) << "pointer geometryField: "
             << this->functionSpace_->geometryField().partitionedPetscVec();
}

std::shared_ptr<typename QuasiStaticNonlinearElasticityFebio::FieldVariableType>
QuasiStaticNonlinearElasticityFebio::activation() {
  return this->activation_;
}

//! return the field variable
std::shared_ptr<
    typename QuasiStaticNonlinearElasticityFebio::FieldVariableTypeVector>
QuasiStaticNonlinearElasticityFebio::referenceGeometry() {
  return this->referenceGeometry_;
}

//! return the field variable
std::shared_ptr<
    typename QuasiStaticNonlinearElasticityFebio::FieldVariableTypeVector>
QuasiStaticNonlinearElasticityFebio::displacements() {
  return this->displacements_;
}

//! return the field variable
std::shared_ptr<
    typename QuasiStaticNonlinearElasticityFebio::FieldVariableTypeVector>
QuasiStaticNonlinearElasticityFebio::reactionForce() {
  return this->reactionForce_;
}

//! return the field variable
std::shared_ptr<
    typename QuasiStaticNonlinearElasticityFebio::FieldVariableTypeTensor>
QuasiStaticNonlinearElasticityFebio::cauchyStress() {
  return this->cauchyStress_;
}

//! return the field variable
std::shared_ptr<
    typename QuasiStaticNonlinearElasticityFebio::FieldVariableTypeTensor>
QuasiStaticNonlinearElasticityFebio::pk2Stress() {
  return this->pk2Stress_;
}

//! return the field variable
std::shared_ptr<
    typename QuasiStaticNonlinearElasticityFebio::FieldVariableTypeTensor>
QuasiStaticNonlinearElasticityFebio::greenLagrangeStrain() {
  return this->greenLagrangeStrain_;
}

//! return the field variable
std::shared_ptr<typename QuasiStaticNonlinearElasticityFebio::FieldVariableType>
QuasiStaticNonlinearElasticityFebio::relativeVolume() {
  return this->relativeVolume_;
}

void QuasiStaticNonlinearElasticityFebio::print() {}

std::shared_ptr<
    typename QuasiStaticNonlinearElasticityFebio::SlotConnectorDataType>
QuasiStaticNonlinearElasticityFebio::getSlotConnectorData() {
  return slotConnectorData_;
}

typename QuasiStaticNonlinearElasticityFebio::FieldVariablesForOutputWriter
QuasiStaticNonlinearElasticityFebio::getFieldVariablesForOutputWriter() {
  // these field variables will be written to output files

  std::shared_ptr<FieldVariableTypeVector> geometryField =
      std::make_shared<FieldVariableTypeVector>(
          this->functionSpace_->geometryField());

  return std::tuple_cat(
      std::tuple<std::shared_ptr<FieldVariableTypeVector>>(geometryField),
      std::tuple<std::shared_ptr<FieldVariableType>>(this->activation_),
      std::tuple<std::shared_ptr<FieldVariableTypeVector>>(
          this->displacements_),
      std::tuple<std::shared_ptr<FieldVariableTypeVector>>(
          this->reactionForce_),
      std::tuple<std::shared_ptr<FieldVariableTypeTensor>>(this->cauchyStress_),
      std::tuple<std::shared_ptr<FieldVariableTypeTensor>>(this->pk2Stress_),
      std::tuple<std::shared_ptr<FieldVariableTypeTensor>>(
          this->greenLagrangeStrain_),
      std::tuple<std::shared_ptr<FieldVariableType>>(this->relativeVolume_));
}

typename QuasiStaticNonlinearElasticityFebio::FieldVariablesForCheckpointing
QuasiStaticNonlinearElasticityFebio::getFieldVariablesForCheckpointing() {
  return this->getFieldVariablesForOutputWriter();
}
} // namespace Data
