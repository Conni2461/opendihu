#include "basis_on_mesh/04_basis_on_mesh_nodes.h"

#include <cmath>
#include <array>

#include "easylogging++.h"
#include "field_variable/field_variable_structured_deformable.h"

namespace BasisOnMesh
{

// constructor
template<int D,typename BasisFunctionType>
BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
BasisOnMeshNodes(PyObject *specificSettings) :
  BasisOnMeshDofs<Mesh::StructuredDeformable<D>,BasisFunctionType>(specificSettings)
{
  std::vector<double> nodePositions;
  this->parseNodePositionsFromSettings(specificSettings, nodePositions);
  this->setGeometryField(nodePositions);
}

template<int D,typename BasisFunctionType>
void BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
initialize()
{
  LOG(DEBUG) << "   retrieve this pointer ";
  std::shared_ptr<BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>> ptr = this->shared_from_this();
  
  assert(ptr != nullptr);
  
  LOG(DEBUG) << "   cast this pointer ";
  std::shared_ptr<BasisOnMesh<Mesh::StructuredDeformable<D>,BasisFunctionType>> self = std::static_pointer_cast<BasisOnMesh<Mesh::StructuredDeformable<D>,BasisFunctionType>>(ptr);
  
  assert(self != nullptr);
  this->geometry_->setMesh(self);
}

// read in config nodes
template<int D,typename BasisFunctionType>
void BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
parseNodePositionsFromSettings(PyObject *specificSettings, std::vector<double> &nodePositions)
{
  // compute number of nodes
  node_idx_t nNodes = this->nNodes();
  
  const int vectorSize = nNodes*3;
  
  // fill initial position from settings
  if (PythonUtility::containsKey(specificSettings, "nodePositions"))
  {
    int nodeDimension = PythonUtility::getOptionInt(specificSettings, "nodeDimension", 3, PythonUtility::ValidityCriterion::Between1And3);
    
    int inputVectorSize = nNodes * nodeDimension;
    PythonUtility::getOptionVector(specificSettings, "nodePositions", inputVectorSize, nodePositions);
    
    LOG(DEBUG) << "nodeDimension: " << nodeDimension << ", expect input vector to have " << inputVectorSize << " entries.";

    // transform vector from (x,y) or (x) entries to (x,y,z) 
    if (nodeDimension < 3)
    {
      nodePositions.resize(vectorSize);   // resize vector and value-initialize to 0
      for(int i=nNodes-1; i>=0; i--)
      {
        
        if (nodeDimension == 2)
          nodePositions[i*3+1] = nodePositions[i*nodeDimension+1];
        else
          nodePositions[i*3+1] = 0;
        nodePositions[i*3+0] = nodePositions[i*nodeDimension+0];
        nodePositions[i*3+2] = 0;
      }
    }
  }
  else
  {
    // if node positions are not given in settings but physicalExtend, fill from that
    std::array<double, D> physicalExtend, meshWidth;
    physicalExtend = PythonUtility::getOptionArray<double, D>(specificSettings, "physicalExtend", 1.0, PythonUtility::Positive);
    
    for (unsigned int dimNo = 0; dimNo < D; dimNo++)
    {
      meshWidth[dimNo] = physicalExtend[dimNo] / this->nElements(dimNo);
      LOG(DEBUG) << "meshWidth["<<dimNo<<"] = "<<meshWidth[dimNo];
    }
    
    std::array<double, 3> position{0.,0.,0.};
    
    nodePositions.resize(vectorSize);   // resize vector and value-initialize to 0
      
    for (node_idx_t nodeNo = 0; nodeNo < nNodes; nodeNo++)
    {
      switch(D)
      {
      case 3:
        position[2] = meshWidth[2] * (int(nodeNo / (this->nNodes(0)*this->nNodes(1))));
      case 2:
        position[1] = meshWidth[1] * (int(nodeNo / this->nNodes(0)) % this->nNodes(1));
      case 1:
        position[0] = meshWidth[0] * (nodeNo % this->nNodes(0));
        
        break;
      }
      
      
      // store the position values in nodePositions
      for (int i=0; i<3; i++)
        nodePositions[nodeNo*3 + i] = position[i];
    }
  }
}

// create geometry field from config nodes
template<int D,typename BasisFunctionType>
void BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
setGeometryField(std::vector<double> &nodePositions)
{
  
  // compute number of dofs
  node_idx_t nDofs = this->nDofs();
  
  LOG(DEBUG) << "setGeometryField, nodePositions: " << nodePositions;
  
  // create petsc vector that contains the node positions
  Vec values;
  PetscErrorCode ierr;
  ierr = VecCreate(PETSC_COMM_WORLD, &values);  CHKERRV(ierr);
  ierr = PetscObjectSetName((PetscObject) values, "geometry"); CHKERRV(ierr);
  
  // initialize size of vector
  const int vectorSize = nDofs * 3;   // dofs always contain 3 entries for every entry (x,y,z)
  ierr = VecSetSizes(values, PETSC_DECIDE, vectorSize); CHKERRV(ierr);
  
  // set sparsity type and other options
  ierr = VecSetFromOptions(values);  CHKERRV(ierr);
  
  // fill geometry vector from nodePositions, initialize non-node position entries to 0 (for Hermite)
  std::vector<double> geometryValues(vectorSize, 0.0);
  
  int geometryValuesIndex = 0;
  int nodePositionsIndex = 0;
  // loop over nodes
  for (node_idx_t nodeNo = 0; nodeNo < this->nNodes(); nodeNo++)
  {
    // assign node position as first dof of the node
    geometryValues[geometryValuesIndex+0] = nodePositions[nodePositionsIndex+0];
    geometryValues[geometryValuesIndex+1] = nodePositions[nodePositionsIndex+1];
    geometryValues[geometryValuesIndex+2] = nodePositions[nodePositionsIndex+2];
    geometryValuesIndex += 3;
    nodePositionsIndex += 3;
    
    // set entries to 0 for rest of dofs at this node
    for (int dofIndex = 1; dofIndex < this->nDofsPerNode(); dofIndex++)
    {
      geometryValues[geometryValuesIndex+0] = 0;
      geometryValues[geometryValuesIndex+1] = 0;
      geometryValues[geometryValuesIndex+2] = 0;
      geometryValuesIndex += 3;
    }
  }
  
  LOG(DEBUG) << "setGeometryField, geometryValues: " << geometryValues;
  
  PetscUtility::setVector(geometryValues, values);
  
  // finish parallel assembly
  ierr = VecAssemblyBegin(values); CHKERRV(ierr);
  ierr = VecAssemblyEnd(values); CHKERRV(ierr);
  
  bool isGeometryField = true;   // if the field is a geometry field
  // set geometry field
  geometry_ = std::make_unique<FieldVariableType>();
  std::vector<std::string> componentNames{"x", "y", "z"};
  int nEntries = nDofs * 3;   // 3 components (x,y,z) per dof
  geometry_->set("geometry", componentNames, this->nElements_, nEntries, isGeometryField, values);
}

template<int D,typename BasisFunctionType>
Vec3 BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
getGeometry(node_idx_t dofGlobalNo) const
{
  Vec3 result = geometry_->template getValue<3>(dofGlobalNo);
  return result;
}  
  
//! return an array containing all geometry entries for an element
template<int D,typename BasisFunctionType>
void BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
getElementGeometry(element_idx_t elementNo, std::array<Vec3, BasisOnMeshBaseDim<D,BasisFunctionType>::nDofsPerElement()> &values)
{
  geometry_->template getElementValues<3>(elementNo, values);
}

template<int D,typename BasisFunctionType>
node_idx_t BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
nNodes() const
{
  int result = 1;
  for (int i=0; i<D; i++)
    result *= nNodes(i);
  return result;
}

template<int D,typename BasisFunctionType>
node_idx_t BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
nNodes(int dimension) const
{
  return this->nElements(dimension) * BasisOnMeshBaseDim<1,BasisFunctionType>::averageNNodesPerElement() + 1;
}

template<int D,typename BasisFunctionType>
node_idx_t BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
nDofs() const
{
  return nNodes() * this->nDofsPerNode();
}

//! return the geometry field
template<int D,typename BasisFunctionType>
typename BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::FieldVariableType &BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
geometryField()
{
  if (this->geometry_ == nullptr)
    LOG(ERROR) << "Geometry field is not yet set.";
  return *this->geometry_;
}

template<int D,typename BasisFunctionType>
void BasisOnMeshNodes<Mesh::StructuredDeformable<D>,BasisFunctionType>::
getNodePositions(std::vector<double> &nodes) const
{
  nodes.resize(this->nNodes()*3);
 
  for (int nodeGlobalNo = 0; nodeGlobalNo < this->nNodes(); nodeGlobalNo++)
  {
   
    node_idx_t firstNodeDofGlobalNo = nodeGlobalNo*this->nDofsPerNode();
    
    int index = nodeGlobalNo*3;
    Vec3 position = this->geometry_->template getValue<3>(firstNodeDofGlobalNo);
    nodes[index+0] = position[0];
    nodes[index+1] = position[1];
    nodes[index+2] = position[2];
  }
}

};  // namespace