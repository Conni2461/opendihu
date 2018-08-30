#pragma once

#include "spatial_discretization/finite_element_method/01_matrix.h"

namespace SpatialDiscretization
{

/**
 * Class that prepares the system to enforce Dirichlet boundary conditions, regular numerical integration
 */
template<typename FunctionSpaceType, typename QuadratureType, typename Term, typename Dummy= Term>
class BoundaryConditions :
  public FiniteElementMethodMatrix<FunctionSpaceType, QuadratureType, Term>
{
public:
  // use constructor of base class
  using FiniteElementMethodMatrix<FunctionSpaceType, QuadratureType, Term>::FiniteElementMethodMatrix;

protected:

  //! apply dirichlet boundary conditions, this calls applyBoundaryConditionsWeakForm
  virtual void applyBoundaryConditions();

  //! apply boundary conditions in weak form by adding a term to the rhs
  void applyBoundaryConditionsWeakForm();
};

/**
 * Class that prepares the system to enforce Dirichlet boundary conditions, when Quadrature::None is given, uses values from stiffness matrix
 */
template<typename FunctionSpaceType, typename Term>
class BoundaryConditions<FunctionSpaceType, Quadrature::None, Term, Term> :
  public FiniteElementMethodMatrix<FunctionSpaceType, Quadrature::None, Term>
{
public:
  // use constructor of base class
  using FiniteElementMethodMatrix<FunctionSpaceType, Quadrature::None, Term>::FiniteElementMethodMatrix;

protected:

  //! apply dirichlet boundary conditions, this calls applyBoundaryConditionsWeakForm
  virtual void applyBoundaryConditions();

  //! Apply Dirichlet BC in strong form by setting columns and rows in stiffness matrix to zero such that Dirichlet boundary conditions are met, sets some rows/columns to 0 and the diagonal to 1, changes rhs accordingly
  void applyBoundaryConditionsStrongForm();
};

/**
 * Partial specialization for solid mechanics, mixed formulation
 */
template<typename LowOrderFunctionSpaceType,typename HighOrderFunctionSpaceType,typename QuadratureType,typename Term>
class BoundaryConditions<FunctionSpace::Mixed<LowOrderFunctionSpaceType,HighOrderFunctionSpaceType>, QuadratureType, Term, Equation::isSolidMechanics<Term>> :
  public FiniteElementMethodMatrix<FunctionSpace::Mixed<LowOrderFunctionSpaceType,HighOrderFunctionSpaceType>, QuadratureType, Term>
{
public:
  // use constructor of base class
  using FiniteElementMethodMatrix<FunctionSpace::Mixed<LowOrderFunctionSpaceType,HighOrderFunctionSpaceType>, QuadratureType, Term>::FiniteElementMethodMatrix;

protected:
  //! apply dirichlet boundary conditions
  void applyBoundaryConditions(){}

};

};  // namespace

#include "spatial_discretization/finite_element_method/02_boundary_conditions_strong_form.tpp"
#include "spatial_discretization/finite_element_method/02_boundary_conditions_weak_form.tpp"
