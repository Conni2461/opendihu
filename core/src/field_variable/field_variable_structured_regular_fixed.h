#pragma once

#include <Python.h>  // has to be the first included header
#include <iostream>
#include <array>
#include <map>

#include "field_variable/field_variable.h"
#include "field_variable/field_variable_base.h"
#include "field_variable/field_variable_interface.h"
#include "field_variable/field_variable_structured.h"
#include "field_variable/component.h"
#include "basis_on_mesh/04_basis_on_mesh_nodes.h"
#include "mesh/unstructured_deformable.h"
#include "field_variable/element_to_node_mapping.h"
#include "field_variable/node_to_dof_mapping.h"

namespace FieldVariable
{

/** FieldVariable class for RegularFixed mesh
 */
template<int D, typename BasisFunctionType>
class FieldVariable<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>> :
  public FieldVariableStructured<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>,
  public Interface<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>
{
public:
  typedef BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType> BasisOnMeshType;
 
  //! inherited constructor 
  using FieldVariableStructured<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>::FieldVariableStructured;
 
  //! set the meshWidth
  void setMeshWidth(double meshWidth);
  
  //! get the mesh width
  double meshWidth() const;
  
  //! for a specific component, get all values
  void getValues(std::string component, std::vector<double> &values)
  {
    if (!this->isGeometryField_)
    {
      FieldVariableStructured<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>::getValues(component, values);
      return;
    }
    
    // for geometry field compute information
    node_no_t nNodesInXDirection = this->mesh_->nNodes(0);
    node_no_t nNodesInYDirection = this->mesh_->nNodes(1);
    node_no_t nNodesInZDirection = this->mesh_->nNodes(2);
    
    if (D < 2)
      nNodesInYDirection = 1;
    if (D < 3)
      nNodesInZDirection = 1;
    
    const int nDofsPerNode = BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>::nDofsPerNode();
   
    LOG(DEBUG) << "getValues, n dofs: " << this->mesh_->nDofs() 
      << ", nNodes: " << nNodesInXDirection<<","<<nNodesInYDirection<<","<<nNodesInZDirection
      << ", nDofsPerNode: " << nDofsPerNode;
    
    values.resize(this->mesh_->nDofs());
    std::size_t vectorIndex = 0;
    
    // loop over all nodes
    for (int nodeZ = 0; nodeZ < nNodesInZDirection; nodeZ++)
    {
      for (int nodeY = 0; nodeY < nNodesInYDirection; nodeY++)
      {
        for (int nodeX = 0; nodeX < nNodesInXDirection; nodeX++)
        {
          if (component == "x")
          {
            assert(vectorIndex < values.size());
            values[vectorIndex++] = nodeX * this->meshWidth_;
          }
          else if (component == "y")
          {
            assert(vectorIndex < values.size());
            values[vectorIndex++] = nodeY * this->meshWidth_;
          }
          else
          {
            assert(vectorIndex < values.size());
            values[vectorIndex++] = nodeZ * this->meshWidth_;
          }
         
          // set derivative of Hermite to 0 for geometry field
          for (int dofIndex = 1; dofIndex < nDofsPerNode; dofIndex++)
          {
            values[vectorIndex++] = 0;
          }
        }
      }
    }
  }
  
  //! for a specific component, get values from their global dof no.s
  template<int N>
  void getValues(std::string component, std::array<dof_no_t,N> dofGlobalNo, std::array<double,N> &values)
  {
    if (!this->isGeometryField_)
    {
      FieldVariableStructured<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>::template getValues<N>(component, dofGlobalNo, values);
      return;
    }
    
    // for geometry field compute information
    const int componentIndex = this->componentIndex_[component];
    
    const node_no_t nNodesInXDirection = this->mesh_->nNodes(0);
    const node_no_t nNodesInYDirection = this->mesh_->nNodes(1);
    const int nDofsPerNode = BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>::nDofsPerNode();
    
   
    // loop over entries in values to be filled
    for (int i=0; i<N; i++)
    {
      int nodeNo = int(dofGlobalNo[i] / nDofsPerNode);
      int nodeLocalDofIndex = int(dofGlobalNo[i] % nDofsPerNode);
      
      if (nodeLocalDofIndex > 0)   // if this is a derivative of Hermite, set to 0
      {
        values[i] = 0;
      }
      else
      {
        if (componentIndex == 0)   // x direction
        {
          values[i] = (nodeNo % nNodesInXDirection) * this->meshWidth_;
        }
        else if (componentIndex == 1)   // y direction
        {
          values[i] = (int(nodeNo / nNodesInXDirection) % nNodesInYDirection) * this->meshWidth_;
        }
        else if (componentIndex == 2)   // z direction
        {
          values[i] = int(nodeNo / (nNodesInXDirection*nNodesInYDirection)) * this->meshWidth_;
        }
      }
    }
  }
  
  //! get values from their global dof no.s for all components
  template<int N, int nComponents>
  void getValues(std::array<dof_no_t,N> dofGlobalNo, std::array<std::array<double,nComponents>,N> &values)
  {
    if (!this->isGeometryField_)
    {
      FieldVariableStructured<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>::template getValues<N,nComponents>(dofGlobalNo, values);
      return;
    }

    const node_no_t nNodesInXDirection = this->mesh_->nNodes(0);
    const node_no_t nNodesInYDirection = this->mesh_->nNodes(1);
    const int nDofsPerNode = BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>::nDofsPerNode();
    
    // loop over entries in values to be filled
    for (int i=0; i<N; i++)
    {
      int nodeNo = int(dofGlobalNo[i] / nDofsPerNode);
      int nodeLocalDofIndex = int(dofGlobalNo[i] % nDofsPerNode);
      
      if (nodeLocalDofIndex > 0)   // if this is a derivative of Hermite, set to 0
      {
        values[i][0] = 0;
        values[i][1] = 0;
        values[i][2] = 0;
      }
      else 
      {
        // x direction
        values[i][0] = (nodeNo % nNodesInXDirection) * this->meshWidth_;
        
        // y direction
        values[i][1] = (int(nodeNo / nNodesInXDirection) % nNodesInYDirection) * this->meshWidth_;
        
        // z direction
        values[i][2] = int(nodeNo / (nNodesInXDirection*nNodesInYDirection)) * this->meshWidth_;
      }
    }
  }
    
  //! for a specific component, get the values corresponding to all element-local dofs
  template<int N>
  void getElementValues(std::string component, element_no_t elementNo, std::array<double,BasisOnMeshType::nDofsPerElement()> &values)
  {
    if (!this->isGeometryField_)
    {
      FieldVariableStructured<BasisOnMeshType>::getElementValues(component, elementNo, values);
      return;
    }
    
    int componentIndex = this->componentIndex_[component];
    const int nDofsPerElement = BasisOnMeshType::nDofsPerElement();
    
    // get the global dof no.s of the element
    std::array<dof_no_t,nDofsPerElement> dofGlobalNo;
    for (int dofIndex = 0; dofIndex < nDofsPerElement; dofIndex++)
    {
      dofGlobalNo[dofIndex] = this->mesh_->getDofNo(elementNo, dofIndex);
    }
    
    this->getValues<nDofsPerElement>(component, dofGlobalNo, values);
  }
  
  //! get the values corresponding to all element-local dofs for all components
  template<std::size_t nComponents>
  void getElementValues(element_no_t elementNo, std::array<std::array<double,nComponents>,BasisOnMeshType::nDofsPerElement()> &values)
  {
    if (!this->isGeometryField_)
    {
      FieldVariableStructured<BasisOnMeshType>::getElementValues(elementNo, values);
      return;
    }
    
    const int nDofsPerElement = BasisOnMeshType::nDofsPerElement();
    
    // get the global dof no.s of the element
    std::array<dof_no_t,nDofsPerElement> dofGlobalNo;
    for (int dofIndex = 0; dofIndex < nDofsPerElement; dofIndex++)
    {
      dofGlobalNo[dofIndex] = this->mesh_->getDofNo(elementNo, dofIndex);
    }
    
    this->getValues<nDofsPerElement,nComponents>(dofGlobalNo, values);
  }
  
  //! for a specific component, get a single value from global dof no.
  double getValue(std::string component, node_no_t dofGlobalNo);

  //! get a single value from global dof no. for all components
  template<std::size_t nComponents>
  std::array<double,nComponents> getValue(node_no_t dofGlobalNo);

  //! copy the values from another field variable of the same type
  void setValues(FieldVariable<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>> &rhs);
  
  //! set values for all components for dofs, after all calls to setValue(s), flushSetValues has to be called to apply the cached changes
  template<std::size_t nComponents>
  void setValues(std::vector<dof_no_t> &dofGlobalNos, std::vector<std::array<double,nComponents>> &values)
  {
    if (!this->isGeometryField_)
    {
      FieldVariableStructured<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>::template setValues<nComponents>(dofGlobalNos, values);
    }
  }

  //! set a single dof (all components) , after all calls to setValue(s), flushSetValues has to be called to apply the cached changes
  template<std::size_t nComponents>
  void setValue(dof_no_t dofGlobalNo, std::array<double,nComponents> &value)
  {
    if (!this->isGeometryField_)
    {
      FieldVariableStructured<BasisOnMesh::BasisOnMesh<Mesh::StructuredRegularFixedOfDimension<D>,BasisFunctionType>>:: template setValue<nComponents>(dofGlobalNo, value);
    }
  }
  
  //! calls PETSc functions to "assemble" the vector, i.e. flush the cached changes
  void flushSetValues();
  
  //! write a exelem file header to a stream, for a particular element
  void outputHeaderExelem(std::ostream &file, element_no_t currentElementGlobalNo, int fieldVariableNo=-1);

  //! write a exelem file header to a stream, for a particular element
  void outputHeaderExnode(std::ostream &file, node_no_t currentNodeGlobalNo, int &valueIndex, int fieldVariableNo=-1);

  //! tell if 2 elements have the same exfile representation, i.e. same number of versions
  bool haveSameExfileRepresentation(element_no_t element1, element_no_t element2);

  //! get the internal PETSc vector values. The meaning of the values is instance-dependent (different for different BasisOnMeshTypes)
  Vec &values();
  
  //! get the number of components
  int nComponents() const;
  
  //! get the number of elements in the coordinate directions
  //std::array<element_no_t, BasisOnMeshType::Mesh::dim()> nElementsPerCoordinateDirection() const;
  
  //! get the total number of elements
  element_no_t nElements() const;
  
  //! get the names of the components that are part of this field variable
  std::vector<std::string> componentNames() const;
  
private:
  double meshWidth_;   ///< the uniform mesh width
};

};  // namespace

#include "field_variable/field_variable_structured_regular_fixed.tpp"
