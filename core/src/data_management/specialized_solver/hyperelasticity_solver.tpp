#include "data_management/specialized_solver/hyperelasticity_solver.h"

#include "specialized_solver/solid_mechanics/hyperelasticity/pressure_function_space_creator.h"

namespace Data {

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::QuasiStaticHyperelasticityBase(DihuContext context)
    : Data<DisplacementsFunctionSpace>::Data(context) {}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                                    DisplacementsFunctionSpace, Term,
                                    withLargeOutput>::initialize() {
  // call initialize of base class, this calls createPetscObjects
  Data<DisplacementsFunctionSpace>::initialize();

  // create the slot connector data object
  slotConnectorData_ = std::make_shared<SlotConnectorDataType>();

  // add all needed field variables to be transferred, here the displacements
  slotConnectorData_->addFieldVariable(this->displacements_, 0);
  slotConnectorData_->addFieldVariable(this->displacements_, 1);
  slotConnectorData_->addFieldVariable(this->displacements_, 2);

  // There is addFieldVariable(...) and addFieldVariable2(...) for the two
  // different field variable types, Refer to
  // "slot_connection/slot_connector_data.h" for details.

  // parse slot names of the field variables, if given
  if (this->context_.getPythonConfig().hasKey("slotNames")) {
    this->context_.getPythonConfig().getOptionVector(
        "slotNames", slotConnectorData_->slotNames);

    // make sure that there are as many slot names as slots
    slotConnectorData_->slotNames.resize(slotConnectorData_->nSlots());
  }
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::FieldVariablesForCheckpointing
QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::getFieldVariablesForCheckpointing() {
  return std::make_tuple(
      std::shared_ptr<DisplacementsFieldVariableType>(
          std::make_shared<
              typename DisplacementsFunctionSpace::GeometryFieldType>(
              this->displacementsFunctionSpace_->geometryField())),
      std::shared_ptr<DisplacementsFieldVariableType>(this->displacements_),

      std::shared_ptr<DisplacementsFieldVariableType>(
          this->displacementsPreviousTimestep_),

      std::shared_ptr<DisplacementsFieldVariableType>(this->velocities_),
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->velocitiesPreviousTimestep_),

      std::shared_ptr<DisplacementsFieldVariableType>(this->fiberDirection_),
      std::shared_ptr<DisplacementsFieldVariableType>(this->traction_),
      std::shared_ptr<DisplacementsFieldVariableType>(this->materialTraction_),

      std::shared_ptr<DisplacementsLinearFieldVariableType>(
          this->displacementsLinearMesh_),
      std::shared_ptr<DisplacementsLinearFieldVariableType>(
          this->velocitiesLinearMesh_),
      std::shared_ptr<PressureFieldVariableType>(this->pressure_),

      std::shared_ptr<StressFieldVariableType>(this->pK2Stress_),
      std::shared_ptr<StressFieldVariableType>(this->activePK2Stress_),

      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->deformationGradient_),
      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->deformationGradientTimeDerivative_));
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
bool QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::restoreState(const InputReader::Generic &r) {
  std::vector<double> displacements, displacementsPT, velocities, velocitiesPT,
      fiberDirection, traction, materialTraction, displacementsLinearMesh,
      velocitiesLinearMesh, pressure, pressurePreviousTimestep, pK2Stress,
      activePK2Stress, deformationGradient, deformationGradientTimeDerivative,
      pK1Stress, cauchyStress, deformationGradientDeterminant;
  if (!r.readDoubleVector(this->displacements_->uniqueName().c_str(),
                          displacements)) {
    return false;
  }
  if (!r.readDoubleVector(
          this->displacementsPreviousTimestep_->uniqueName().c_str(),
          displacementsPT)) {
    return false;
  }

  if (!r.readDoubleVector(this->velocities_->uniqueName().c_str(),
                          velocities)) {
    return false;
  }
  if (!r.readDoubleVector(
          this->velocitiesPreviousTimestep_->uniqueName().c_str(),
          velocitiesPT)) {
    return false;
  }

  if (!r.readDoubleVector(this->fiberDirection_->uniqueName().c_str(),
                          fiberDirection)) {
    return false;
  }
  if (!r.readDoubleVector(this->traction_->uniqueName().c_str(), traction)) {
    return false;
  }
  if (!r.readDoubleVector(this->materialTraction_->uniqueName().c_str(),
                          materialTraction)) {
    return false;
  }

  if (!r.readDoubleVector(this->displacementsLinearMesh_->uniqueName().c_str(),
                          displacementsLinearMesh)) {
    return false;
  }
  if (!r.readDoubleVector(this->velocitiesLinearMesh_->uniqueName().c_str(),
                          velocitiesLinearMesh)) {
    return false;
  }
  if (!r.readDoubleVector(this->pressure_->uniqueName().c_str(), pressure)) {
    return false;
  }
  bool hasPressurePreviousTimestep =
      pressurePreviousTimestep_
          ? r.readDoubleVector(
                this->pressurePreviousTimestep_->uniqueName().c_str(),
                pressurePreviousTimestep)
          : false;

  if (!r.readDoubleVector(this->pK2Stress_->uniqueName().c_str(), pK2Stress)) {
    return false;
  }
  if (!r.readDoubleVector(this->activePK2Stress_->uniqueName().c_str(),
                          activePK2Stress)) {
    return false;
  }

  if (!r.readDoubleVector(this->deformationGradient_->uniqueName().c_str(),
                          deformationGradient)) {
    return false;
  }
  if (!r.readDoubleVector(
          this->deformationGradientTimeDerivative_->uniqueName().c_str(),
          deformationGradientTimeDerivative)) {
    return false;
  }

  bool haspK1Stress =
      r.readDoubleVector(this->pK1Stress_->uniqueName().c_str(), pK1Stress);
  bool hasCauchyStress = r.readDoubleVector(
      this->cauchyStress_->uniqueName().c_str(), cauchyStress);
  bool hasDeformationGradientDeterminant_ = r.readDoubleVector(
      this->deformationGradientDeterminant_->uniqueName().c_str(),
      deformationGradientDeterminant);

  this->displacements_->setValues(displacements);
  this->displacementsPreviousTimestep_->setValues(displacementsPT);

  this->velocities_->setValues(velocities);
  this->velocitiesPreviousTimestep_->setValues(velocitiesPT);

  this->fiberDirection_->setValues(fiberDirection);
  this->traction_->setValues(traction);
  this->materialTraction_->setValues(materialTraction);

  this->displacementsLinearMesh_->setValues(displacementsLinearMesh);
  this->velocitiesLinearMesh_->setValues(velocitiesLinearMesh);
  this->pressure_->setValues(pressure);
  if (hasPressurePreviousTimestep && pressurePreviousTimestep_) {
    pressurePreviousTimestep_->setValues(pressurePreviousTimestep);
  }

  this->pK2Stress_->setValues(pK2Stress);

  this->deformationGradient_->setValues(deformationGradient);
  this->deformationGradientTimeDerivative_->setValues(
      deformationGradientTimeDerivative);

  if (haspK1Stress) {
    this->pK1Stress_->setValues(pK1Stress);
  }
  if (hasCauchyStress) {
    this->cauchyStress_->setValues(cauchyStress);
  }
  if (hasDeformationGradientDeterminant_) {
    this->deformationGradientDeterminant_->setValues(
        deformationGradientDeterminant);
  }

  // TODO(conni2461): restore geometry
  return true;
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                                    DisplacementsFunctionSpace, Term,
                                    withLargeOutput>::createPetscObjects() {
  LOG(DEBUG) << "QuasiStaticHyperelasticityBase::createPetscObject";

  assert(this->displacementsFunctionSpace_);
  assert(this->pressureFunctionSpace_);
  assert(this->functionSpace_);

  std::vector<std::string> displacementsComponentNames({"x", "y", "z"});

  displacements_ =
      this->displacementsFunctionSpace_->template createFieldVariable<3>(
          "u", displacementsComponentNames);
  displacements_->setUniqueName("hyperelasticity_solver_u");
  displacementsPreviousTimestep_ =
      this->displacementsFunctionSpace_->template createFieldVariable<3>(
          "u_previous", displacementsComponentNames);
  displacementsPreviousTimestep_->setUniqueName(
      "hyperelasticity_solver_u_previous");

  velocities_ =
      this->displacementsFunctionSpace_->template createFieldVariable<3>(
          "v", displacementsComponentNames);
  velocities_->setUniqueName("hyperelasticity_solver_v");
  velocitiesPreviousTimestep_ =
      this->displacementsFunctionSpace_->template createFieldVariable<3>(
          "v_previous", displacementsComponentNames);
  velocitiesPreviousTimestep_->setUniqueName(
      "hyperelasticity_solver_v_previous");

  fiberDirection_ =
      this->displacementsFunctionSpace_->template createFieldVariable<3>(
          "fiberDirection", displacementsComponentNames);
  fiberDirection_->setUniqueName("hyperelasticity_solver_fiberDirection");
  traction_ =
      this->displacementsFunctionSpace_->template createFieldVariable<3>(
          "t (current traction)", displacementsComponentNames);
  traction_->setUniqueName("hyperelasticity_solver_t (current traction)");
  materialTraction_ =
      this->displacementsFunctionSpace_->template createFieldVariable<3>(
          "T (material traction)", displacementsComponentNames);
  materialTraction_->setUniqueName(
      "hyperelasticity_solver_T (material traction)");
  displacementsLinearMesh_ =
      this->pressureFunctionSpace_->template createFieldVariable<3>(
          "uLin", displacementsComponentNames); //< u, the displacements
  displacementsLinearMesh_->setUniqueName(
      "hyperelasticity_solver_uLin"); //< u, the displacements
  velocitiesLinearMesh_ =
      this->pressureFunctionSpace_->template createFieldVariable<3>(
          "vLin", displacementsComponentNames); //< v, the velocities
  velocitiesLinearMesh_->setUniqueName(
      "hyperelasticity_solver_vLin"); //< v, the velocities
  pressure_ = this->pressureFunctionSpace_->template createFieldVariable<1>(
      "p"); //<  p, the pressure variable
  pressure_->setUniqueName(
      "hyperelasticity_solver_p"); //<  p, the pressure variable
  if (Term::isIncompressible) {
    pressurePreviousTimestep_ =
        this->pressureFunctionSpace_->template createFieldVariable<1>(
            "p_previous"); //<  p, the pressure variable
    pressurePreviousTimestep_->setUniqueName(
        "hyperelasticity_solver_p_previous"); //<  p, the pressure variable
  } else {
    pressurePreviousTimestep_ = nullptr;
  }

  std::vector<std::string> componentNamesS{
      "S_11", "S_22", "S_33",
      "S_12", "S_23", "S_13"}; // component names in Voigt notation
  pK2Stress_ =
      this->displacementsFunctionSpace_->template createFieldVariable<6>(
          "PK2-Stress (Voigt)", componentNamesS); //<  the symmetric PK2 stress
                                                  // tensor in Voigt notation
  pK2Stress_->setUniqueName("hyperelasticity_solver_PK2-Stress (Voigt)");
  activePK2Stress_ =
      this->displacementsFunctionSpace_->template createFieldVariable<6>(
          "active PK2-Stress (Voigt)",
          componentNamesS); //<  the symmetric active PK2 stress tensor in Voigt
                            // notation
  activePK2Stress_->setUniqueName(
      "hyperelasticity_solver_active PK2-Stress (Voigt)");

  std::vector<std::string> componentNamesF{
      "F_11", "F_12", "F_13", "F_21", "F_22", "F_23", "F_31", "F_32", "F_33"};
  deformationGradient_ =
      this->displacementsFunctionSpace_->template createFieldVariable<9>(
          "F", componentNamesF);
  deformationGradient_->setUniqueName("hyperelasticity_solver_F");
  deformationGradientTimeDerivative_ =
      this->displacementsFunctionSpace_->template createFieldVariable<9>(
          "Fdot", componentNamesF);
  deformationGradientTimeDerivative_->setUniqueName(
      "hyperelasticity_solver_Fdot");

  if (withLargeOutput) {
    std::vector<std::string> componentNamesP{
        "P_11", "P_12", "P_13", "P_21", "P_22", "P_23", "P_31", "P_32", "P_33"};
    pK1Stress_ =
        this->displacementsFunctionSpace_->template createFieldVariable<9>(
            "P (PK1 stress)", componentNamesP);
    pK1Stress_->setUniqueName("hyperelasticity_solver_P (PK1 stress)");
    std::vector<std::string> componentNamesSigma{
        "σ_11", "σ_12", "σ_13", "σ_21", "σ_22", "σ_23", "σ_31", "σ_32", "σ_33"};
    cauchyStress_ =
        this->displacementsFunctionSpace_->template createFieldVariable<9>(
            "σ (Cauchy stress)", componentNamesSigma);
    cauchyStress_->setUniqueName("hyperelasticity_solver_σ (Cauchy stress)");
    deformationGradientDeterminant_ =
        this->displacementsFunctionSpace_->template createFieldVariable<1>(
            "J"); // J=det(F)
    deformationGradientDeterminant_->setUniqueName("hyperelasticity_solver_J");
  }
}

//! field variable of geometryReference_
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::geometryReference() {
  return this->geometryReference_;
}

//! field variable of u or u^(n+1)
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::displacements() {
  return this->displacements_;
}
//! field variable of u^(n)
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType>
QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::displacementsPreviousTimestep() {
  return this->displacementsPreviousTimestep_;
}

//! field variable of v or v^(n+1)
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::velocities() {
  return this->velocities_;
}
//! field variable of v^(n)
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::velocitiesPreviousTimestep() {
  return this->velocitiesPreviousTimestep_;
}

//! field variable of fiber direction
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType> &
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::fiberDirection() {
  return this->fiberDirection_;
}

//! field variable of current configuration traction
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::traction() {
  return this->traction_;
}

//! field variable of material traction
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::materialTraction() {
  return this->materialTraction_;
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::SlotConnectorDataType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::getSlotConnectorData() {
  // return the slot connector data object
  return this->slotConnectorData_;
}

//! field variable displacements u but on the linear mesh
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsLinearFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::displacementsLinearMesh() {
  return this->displacementsLinearMesh_;
}

//! field variable velocities v but on the linear mesh
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DisplacementsLinearFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::velocitiesLinearMesh() {
  return this->velocitiesLinearMesh_;
}

//! field variable of p or p^(n+1)
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::PressureFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::pressure() {
  return this->pressure_;
}

//! field variable of p^(n)
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::PressureFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::pressurePreviousTimestep() {
  return this->pressurePreviousTimestep_;
}

//! field variable of S
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::StressFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::pK2Stress() {
  return this->pK2Stress_;
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::StressFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::activePK2Stress() {
  return this->activePK2Stress_;
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DeformationGradientFieldVariableType>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::deformationGradient() {
  return this->deformationGradient_;
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<typename QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::DeformationGradientFieldVariableType>
QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::deformationGradientTimeDerivative() {
  return this->deformationGradientTimeDerivative_;
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::updateGeometry(double scalingFactor,
                                     bool updateLinearVariables) {
  VLOG(1) << "updateGeometry, scalingFactor=" << scalingFactor
          << ", updateLinearVariables: " << updateLinearVariables;
  PetscErrorCode ierr;

  this->displacementsFunctionSpace_->geometryField().finishGhostManipulation();

  // update quadratic function space geometry
  // w = alpha * x + y, VecWAXPY(w, alpha, x, y)
  ierr = VecWAXPY(
      this->displacementsFunctionSpace_->geometryField().valuesGlobal(),
      scalingFactor, this->displacements_->valuesGlobal(),
      this->geometryReference_->valuesGlobal());
  CHKERRV(ierr);

  this->displacementsFunctionSpace_->geometryField().startGhostManipulation();

  VLOG(1) << "update done.";
  VLOG(1) << "displacements representation: "
          << this->displacements_->partitionedPetscVec()
                 ->getCurrentRepresentationString();
  VLOG(1) << "geometryReference_ representation: "
          << this->geometryReference_->partitionedPetscVec()
                 ->getCurrentRepresentationString();
  VLOG(1) << "displacementsFunctionSpace_ representation: "
          << this->displacementsFunctionSpace_->geometryField()
                 .partitionedPetscVec()
                 ->getCurrentRepresentationString();

  // if the linear variables (geometry, displacements, velocities) should be
  // updated in order to output with the pressure output writer
  if (updateLinearVariables) {
    // for displacements extract linear mesh from quadratic mesh
    std::vector<Vec3> displacementValues;
    this->displacements_->getValuesWithGhosts(displacementValues);

    std::vector<Vec3> velocityValues;
    this->velocities_->getValuesWithGhosts(velocityValues);

    std::vector<Vec3> linearMeshDisplacementValues;
    std::vector<Vec3> linearMeshVelocityValues;

    ::SpatialDiscretization::PressureFunctionSpaceCreator<
        typename PressureFunctionSpace::Mesh>::
        extractPressureFunctionSpaceValues(
            this->displacementsFunctionSpace_, this->pressureFunctionSpace_,
            displacementValues, linearMeshDisplacementValues);

    ::SpatialDiscretization::PressureFunctionSpaceCreator<
        typename PressureFunctionSpace::Mesh>::
        extractPressureFunctionSpaceValues(
            this->displacementsFunctionSpace_, this->pressureFunctionSpace_,
            velocityValues, linearMeshVelocityValues);

    displacementsLinearMesh_->setValuesWithGhosts(linearMeshDisplacementValues,
                                                  INSERT_VALUES);
    velocitiesLinearMesh_->setValuesWithGhosts(linearMeshVelocityValues,
                                               INSERT_VALUES);

    // update linear function space geometry
    this->pressureFunctionSpace_->geometryField().finishGhostManipulation();

    // w = alpha * x + y, VecWAXPY(w, alpha, x, y)
    ierr =
        VecWAXPY(this->pressureFunctionSpace_->geometryField().valuesGlobal(),
                 1, this->displacementsLinearMesh_->valuesGlobal(),
                 this->geometryReferenceLinearMesh_->valuesGlobal());
    CHKERRV(ierr);

    this->pressureFunctionSpace_->geometryField().startGhostManipulation();
  }
}

//! set the function space object that discretizes the pressure field variable
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term, withLargeOutput>::
    setPressureFunctionSpace(
        std::shared_ptr<PressureFunctionSpace> pressureFunctionSpace) {
  pressureFunctionSpace_ = pressureFunctionSpace;

  // set the geometry field of the reference configuration as copy of the
  // geometry field of the function space
  geometryReferenceLinearMesh_ =
      std::make_shared<DisplacementsLinearFieldVariableType>(
          pressureFunctionSpace_->geometryField(),
          "geometryReferenceLinearMesh");
  geometryReferenceLinearMesh_->setValues(
      pressureFunctionSpace_->geometryField());

  // communicate ghost values
  geometryReferenceLinearMesh_->setRepresentationGlobal();
  geometryReferenceLinearMesh_->startGhostManipulation();
  geometryReferenceLinearMesh_->setRepresentationGlobal();
}

//! set the function space object that discretizes the displacements field
//! variable
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term, withLargeOutput>::
    setDisplacementsFunctionSpace(std::shared_ptr<DisplacementsFunctionSpace>
                                      displacementsFunctionSpace) {
  displacementsFunctionSpace_ = displacementsFunctionSpace;

  // also set the functionSpace_ variable which is from the parent class Data
  this->functionSpace_ = displacementsFunctionSpace;

  LOG(DEBUG) << "create geometry Reference";

  // set the geometry field of the reference configuration as copy of the
  // geometry field of the function space
  geometryReference_ = std::make_shared<DisplacementsFieldVariableType>(
      displacementsFunctionSpace_->geometryField(), "geometryReference");
  geometryReference_->setValues(displacementsFunctionSpace_->geometryField());

  // communicate ghost values
  geometryReference_->setRepresentationGlobal();
  geometryReference_->startGhostManipulation();
  geometryReference_->setRepresentationGlobal();
}

//! update the copy of the reference geometry field from the current geometry
//! field of the displacementsFunctionSpace
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term,
    withLargeOutput>::updateReferenceGeometry() {
  assert(geometryReference_);
  assert(displacementsFunctionSpace_);

  // assign the reference values
  geometryReference_->setValues(displacementsFunctionSpace_->geometryField());
  geometryReferenceLinearMesh_->setValues(
      pressureFunctionSpace_->geometryField());

  // communicate ghost values
  geometryReference_->setRepresentationGlobal();
  geometryReference_->startGhostManipulation();
  geometryReference_->setRepresentationGlobal();

  geometryReferenceLinearMesh_->setRepresentationGlobal();
  geometryReferenceLinearMesh_->startGhostManipulation();
  geometryReferenceLinearMesh_->setRepresentationGlobal();
}

//! get the displacements function space
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<DisplacementsFunctionSpace>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::displacementsFunctionSpace() {
  if (!displacementsFunctionSpace_)
    LOG(FATAL) << "displacementsFunctionSpace is not set!";
  return displacementsFunctionSpace_;
}

//! get the pressure function space
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
std::shared_ptr<PressureFunctionSpace>
QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                               DisplacementsFunctionSpace, Term,
                               withLargeOutput>::pressureFunctionSpace() {
  if (!pressureFunctionSpace_)
    LOG(FATAL) << "pressureFunctionSpace is not set!";
  return pressureFunctionSpace_;
}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                                    DisplacementsFunctionSpace, Term,
                                    withLargeOutput>::print() {}

template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput>
void QuasiStaticHyperelasticityBase<PressureFunctionSpace,
                                    DisplacementsFunctionSpace, Term,
                                    withLargeOutput>::computePk1Stress() {
  std::vector<VecD<9>> deformationGradientValues;
  std::vector<VecD<6>> pK2StressValues;

  this->deformationGradient_->getValuesWithoutGhosts(deformationGradientValues);
  this->pK2Stress_->getValuesWithoutGhosts(pK2StressValues);

  // loop over all local entries
  for (dof_no_t dofNoLocal = 0;
       dofNoLocal <
       this->displacementsFunctionSpace_->nDofsLocalWithoutGhosts();
       dofNoLocal++) {
    // convert to column-major matrices
    // deformation gradient F
    double Fxx = deformationGradientValues[dofNoLocal][0];
    double Fxy = deformationGradientValues[dofNoLocal][1];
    double Fxz = deformationGradientValues[dofNoLocal][2];
    double Fyx = deformationGradientValues[dofNoLocal][3];
    double Fyy = deformationGradientValues[dofNoLocal][4];
    double Fyz = deformationGradientValues[dofNoLocal][5];
    double Fzx = deformationGradientValues[dofNoLocal][6];
    double Fzy = deformationGradientValues[dofNoLocal][7];
    double Fzz = deformationGradientValues[dofNoLocal][8];
    Tensor2<3> deformationGradient{Vec3{Fxx, Fyx, Fzx}, Vec3{Fxy, Fyy, Fzy},
                                   Vec3{Fxz, Fyz, Fzz}};

    // PK2 stress tensor S (symmetric)
    double Sx = pK2StressValues[dofNoLocal][0];
    double Sy = pK2StressValues[dofNoLocal][1];
    double Sz = pK2StressValues[dofNoLocal][2];
    double Sxy = pK2StressValues[dofNoLocal][3];
    double Syz = pK2StressValues[dofNoLocal][4];
    double Sxz = pK2StressValues[dofNoLocal][5];
    Tensor2<3> pK2Stress{Vec3{Sx, Sxy, Sxz}, Vec3{Sxy, Sy, Syz},
                         Vec3{Sxz, Syz, Sz}};

    // compute PK1 stress tensor P = F*S (unsymmetric)
    Tensor2<3> pK1Stress = deformationGradient * pK2Stress;

    // store resulting value of the PK1 stress tensor (row-major)
    this->pK1Stress_->setValue(
        dofNoLocal, VecD<9>{pK1Stress[0][0], pK1Stress[1][0], pK1Stress[2][0],
                            pK1Stress[0][1], pK1Stress[1][1], pK1Stress[2][1],
                            pK1Stress[0][2], pK1Stress[1][2], pK1Stress[2][2]});

    // compute J = determinant of F
    double detF = MathUtility::computeDeterminant(deformationGradient);
    this->deformationGradientDeterminant_->setValue(dofNoLocal, detF);

    // compute Cauchy stress σ = J^-1 P F^T (unsymmetric)
    Tensor2<3> cauchyStress =
        1. / detF * pK1Stress *
        MathUtility::computeTranspose(deformationGradient);
    if (fabs(detF) < 1e-12) {
      detF = 3;
      cauchyStress = Tensor2<3>{};
    }

    // store resulting value of the cauchy stress tensor (row-major)
    this->cauchyStress_->setValue(
        dofNoLocal,
        VecD<9>{cauchyStress[0][0], cauchyStress[1][0], cauchyStress[2][0],
                cauchyStress[0][1], cauchyStress[1][1], cauchyStress[2][1],
                cauchyStress[0][2], cauchyStress[1][2], cauchyStress[2][2]});
  }
}

// withLargeOutput = false, Term::usesFiberDirection = false
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term, bool withLargeOutput, typename DummyForTraits>
typename QuasiStaticHyperelasticity<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term, withLargeOutput,
    DummyForTraits>::FieldVariablesForOutputWriter
QuasiStaticHyperelasticity<PressureFunctionSpace, DisplacementsFunctionSpace,
                           Term, withLargeOutput,
                           DummyForTraits>::getFieldVariablesForOutputWriter() {
  LOG(DEBUG) << "getFieldVariablesForOutputWriter, without fiberDirection";

  // these field variables will be written to output files
  return std::make_tuple(
      std::shared_ptr<DisplacementsFieldVariableType>(
          std::make_shared<
              typename DisplacementsFunctionSpace::GeometryFieldType>(
              this->displacementsFunctionSpace_->geometryField())), // geometry
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->displacements_), // displacements_
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->velocities_), // velocities_
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->materialTraction_), // materialTraction_
      std::shared_ptr<StressFieldVariableType>(this->pK2Stress_) // pK2Stress_
  );

  /*
  // code to output the pressure field variables
  return std::tuple_cat(
    std::tuple<std::shared_ptr<DisplacementsLinearFieldVariableType>>(std::make_shared<typename
  PressureFunctionSpace::GeometryFieldType>(this->pressureFunctionSpace_->geometryField())),
  // geometry
    std::tuple<std::shared_ptr<PressureFieldVariableType>>(this->pressure_)
  );
  */
}

// withLargeOutput = true, Term::usesFiberDirection = false
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term>
typename QuasiStaticHyperelasticity<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term, true,
    std::enable_if_t<!Term::usesFiberDirection,
                     Term>>::FieldVariablesForOutputWriter
QuasiStaticHyperelasticity<PressureFunctionSpace, DisplacementsFunctionSpace,
                           Term, true,
                           std::enable_if_t<!Term::usesFiberDirection, Term>>::
    getFieldVariablesForOutputWriter() {
  this->computePk1Stress();

  LOG(DEBUG) << "getFieldVariablesForOutputWriter, without fiberDirection";

  // these field variables will be written to output files
  return std::make_tuple(
      std::shared_ptr<DisplacementsFieldVariableType>(
          std::make_shared<
              typename DisplacementsFunctionSpace::GeometryFieldType>(
              this->displacementsFunctionSpace_->geometryField())), // geometry
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->displacements_), // displacements_
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->velocities_), // velocities_
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->traction_), // traction_
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->materialTraction_), // materialTraction_
      std::shared_ptr<StressFieldVariableType>(this->pK2Stress_), // pK2Stress_
      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->deformationGradient_),
      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->deformationGradientTimeDerivative_),
      std::shared_ptr<DeformationGradientFieldVariableType>(this->pK1Stress_),
      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->cauchyStress_),
      std::shared_ptr<
          FieldVariable::FieldVariable<DisplacementsFunctionSpace, 1>>(
          this->deformationGradientDeterminant_));
}

// withLargeOutput = false, Term::usesFiberDirection = true
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term>
typename QuasiStaticHyperelasticity<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term, false,
    std::enable_if_t<Term::usesFiberDirection,
                     Term>>::FieldVariablesForOutputWriter
QuasiStaticHyperelasticity<PressureFunctionSpace, DisplacementsFunctionSpace,
                           Term, false,
                           std::enable_if_t<Term::usesFiberDirection, Term>>::
    getFieldVariablesForOutputWriter() {
  LOG(DEBUG) << "getFieldVariablesForOutputWriter, with fiberDirection";
  return std::make_tuple(
      std::shared_ptr<DisplacementsFieldVariableType>(
          std::make_shared<
              typename DisplacementsFunctionSpace::GeometryFieldType>(
              this->displacementsFunctionSpace_->geometryField())), // geometry
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->displacements_), // displacements_
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->velocities_),                                     // velocities_
      std::shared_ptr<StressFieldVariableType>(this->pK2Stress_), // pK2Stress_
      std::shared_ptr<StressFieldVariableType>(
          this->activePK2Stress_), // activePK2Stress_
      std::shared_ptr<DisplacementsFieldVariableType>(this->fiberDirection_),
      std::shared_ptr<DisplacementsFieldVariableType>(this->materialTraction_)

  );
}

// withLargeOutput = true, Term::usesFiberDirection = true
template <typename PressureFunctionSpace, typename DisplacementsFunctionSpace,
          typename Term>
typename QuasiStaticHyperelasticity<
    PressureFunctionSpace, DisplacementsFunctionSpace, Term, true,
    std::enable_if_t<Term::usesFiberDirection,
                     Term>>::FieldVariablesForOutputWriter
QuasiStaticHyperelasticity<PressureFunctionSpace, DisplacementsFunctionSpace,
                           Term, true,
                           std::enable_if_t<Term::usesFiberDirection, Term>>::
    getFieldVariablesForOutputWriter() {
  this->computePk1Stress();

  LOG(DEBUG) << "getFieldVariablesForOutputWriter, with fiberDirection";
  return std::make_tuple(
      std::shared_ptr<DisplacementsFieldVariableType>(
          std::make_shared<
              typename DisplacementsFunctionSpace::GeometryFieldType>(
              this->displacementsFunctionSpace_->geometryField())), // geometry
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->displacements_), // displacements_
      std::shared_ptr<DisplacementsFieldVariableType>(
          this->velocities_),                                     // velocities_
      std::shared_ptr<StressFieldVariableType>(this->pK2Stress_), // pK2Stress_
      std::shared_ptr<StressFieldVariableType>(
          this->activePK2Stress_), // activePK2Stress_
      std::shared_ptr<DisplacementsFieldVariableType>(this->fiberDirection_),
      std::shared_ptr<DisplacementsFieldVariableType>(this->traction_),
      std::shared_ptr<DisplacementsFieldVariableType>(this->materialTraction_),
      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->deformationGradient_),
      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->deformationGradientTimeDerivative_),
      std::shared_ptr<DeformationGradientFieldVariableType>(this->pK1Stress_),
      std::shared_ptr<DeformationGradientFieldVariableType>(
          this->cauchyStress_),
      std::shared_ptr<
          FieldVariable::FieldVariable<DisplacementsFunctionSpace, 1>>(
          this->deformationGradientDeterminant_));
}

// --------------------------------
// QuasiStaticHyperelasticityPressureOutput
template <typename PressureFunctionSpace>
void QuasiStaticHyperelasticityPressureOutput<PressureFunctionSpace>::
    initialize(
        std::shared_ptr<typename QuasiStaticHyperelasticityPressureOutput<
            PressureFunctionSpace>::PressureFieldVariableType>
            pressure,
        std::shared_ptr<typename QuasiStaticHyperelasticityPressureOutput<
            PressureFunctionSpace>::DisplacementsLinearFieldVariableType>
            displacementsLinearMesh,
        std::shared_ptr<typename QuasiStaticHyperelasticityPressureOutput<
            PressureFunctionSpace>::DisplacementsLinearFieldVariableType>
            velocitiesLinearMesh) {
  pressure_ = pressure;
  displacementsLinearMesh_ = displacementsLinearMesh;
  velocitiesLinearMesh_ = velocitiesLinearMesh;
}

template <typename PressureFunctionSpace>
typename QuasiStaticHyperelasticityPressureOutput<
    PressureFunctionSpace>::FieldVariablesForOutputWriter
QuasiStaticHyperelasticityPressureOutput<
    PressureFunctionSpace>::getFieldVariablesForOutputWriter() {
  // these field variables will be written to output files
  return std::tuple_cat(
      std::tuple<std::shared_ptr<DisplacementsLinearFieldVariableType>>(
          std::make_shared<typename PressureFunctionSpace::GeometryFieldType>(
              this->functionSpace_->geometryField())), // geometry
      std::tuple<std::shared_ptr<DisplacementsLinearFieldVariableType>>(
          this->displacementsLinearMesh_),
      std::tuple<std::shared_ptr<DisplacementsLinearFieldVariableType>>(
          this->velocitiesLinearMesh_),
      std::tuple<std::shared_ptr<PressureFieldVariableType>>(this->pressure_));
}

template <typename PressureFunctionSpace>
typename QuasiStaticHyperelasticityPressureOutput<
    PressureFunctionSpace>::FieldVariablesForCheckpointing
QuasiStaticHyperelasticityPressureOutput<
    PressureFunctionSpace>::getFieldVariablesForCheckpointing() {
  return this->getFieldVariablesForOutputWriter();
}

template <typename PressureFunctionSpace>
bool QuasiStaticHyperelasticityPressureOutput<
    PressureFunctionSpace>::restoreState(const InputReader::Generic &r) {
  std::vector<double> displacementsLinearMesh, velocitiesLinearMesh, pressure;
  if (!r.readDoubleVector(this->displacementsLinearMesh_->uniqueName().c_str(),
                          displacementsLinearMesh)) {
    return false;
  }
  if (!r.readDoubleVector(this->velocitiesLinearMesh_->uniqueName().c_str(),
                          velocitiesLinearMesh)) {
    return false;
  }
  if (!r.readDoubleVector(this->pressure_->uniqueName().c_str(), pressure)) {
    return false;
  }

  this->displacementsLinearMesh_->setValues(displacementsLinearMesh);
  this->velocitiesLinearMesh_->setValues(velocitiesLinearMesh);
  this->pressure_->setValues(pressure);
  // TODO(conni2461): restore geometry
  return true;
}

} // namespace Data
