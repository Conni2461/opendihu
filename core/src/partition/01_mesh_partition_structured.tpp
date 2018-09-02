#include "partition/01_mesh_partition.h"

#include "utility/vector_operators.h"
#include "function_space/00_function_space_base_dim.h"

namespace Partition
{
  
template<typename MeshType,typename BasisFunctionType>
MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
MeshPartition(std::array<global_no_t,MeshType::dim()> nElementsGlobal, std::shared_ptr<RankSubset> rankSubset) :
  MeshPartitionBase(rankSubset), nElementsGlobal_(nElementsGlobal), hasFullNumberOfNodes_({false})
{
  VLOG(1) << "create MeshPartition where only the global size is known, " 
    << "nElementsGlobal: " << nElementsGlobal_ << ", rankSubset: " << *rankSubset << ", mesh dimension: " << MeshType::dim();
  
  this->createDmElements();
  this->createLocalDofOrderings();
  
  LOG(DEBUG) << "nElementsLocal_: " << nElementsLocal_ << ", nElementsGlobal_: " << nElementsGlobal_
    << ", hasFullNumberOfNodes_: " << hasFullNumberOfNodes_;
  LOG(DEBUG) << "nRanks: " << nRanks_ << ", localSizesOnRanks_: " << localSizesOnRanks_ << ", beginElementGlobal_: " << beginElementGlobal_;
  
  LOG(DEBUG) << *this;
}

template<typename MeshType,typename BasisFunctionType>
MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
MeshPartition(std::array<node_no_t,MeshType::dim()> nElementsLocal, std::array<global_no_t,MeshType::dim()> nElementsGlobal, 
              std::array<int,MeshType::dim()> beginElementGlobal, 
              std::array<int,MeshType::dim()> nRanks, std::shared_ptr<RankSubset> rankSubset) :
  MeshPartitionBase(rankSubset), beginElementGlobal_(beginElementGlobal), nElementsLocal_(nElementsLocal), nElementsGlobal_(nElementsGlobal), 
  nRanks_(nRanks), hasFullNumberOfNodes_({false})
{
  // partitioning is already prescribed as every rank knows its own local size
 
  VLOG(1) << "create MeshPartition where every rank already knows its own local size. " 
    << "nElementsLocal: " << nElementsLocal << ", nElementsGlobal: " << nElementsGlobal 
    << ", beginElementGlobal: " << beginElementGlobal << ", nRanks: " << nRanks << ", rankSubset: " << *rankSubset;
    
  initializeHasFullNumberOfNodes();
  
  // determine localSizesOnRanks_
  for (int i = 0; i < MeshType::dim(); i++)
  {
    localSizesOnRanks_[i].resize(rankSubset->size());
  }
  
  for (int i = 0; i < MeshType::dim(); i++)
  {
    MPIUtility::handleReturnValue(MPI_Allgather(&nElementsLocal_[i], 1, MPI_INT, 
      localSizesOnRanks_[i].data(), rankSubset->size(), MPI_INT, rankSubset->mpiCommunicator()));
  }
  VLOG(1) << "determined localSizesOnRanks: " << localSizesOnRanks_;

  this->createLocalDofOrderings();
  
  LOG(DEBUG) << *this;
  
  for (int i = 0; i < MeshType::dim(); i++)
  {
    LOG(DEBUG) << "  beginNodeGlobalNatural(" << i << "): " << beginNodeGlobalNatural(i);
  }
}

template<typename MeshType,typename BasisFunctionType>
void MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
initializeHasFullNumberOfNodes()
{
  // determine if the local partition is at the x+/y+/z+ border of the global domain
  for (int i = 0; i < MeshType::dim(); i++)
  {
    assert (beginElementGlobal_[i] + nElementsLocal_[i] <= (int)nElementsGlobal_[i]);
    
    if (beginElementGlobal_[i] + nElementsLocal_[i] == (int)nElementsGlobal_[i])
    {
      hasFullNumberOfNodes_[i] = true;
    }
  }
}

template<typename MeshType,typename BasisFunctionType>
void MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
createDmElements()
{
  dmElements_ = std::make_shared<DM>();
  
  PetscErrorCode ierr;
  const int ghostLayerWidth = 1;
  const int nDofsPerElement = 1;   // a multiplicity parameter to the items for which the partitioning is generated by PETSc. 
  
  // create PETSc DMDA object that is a topology interface handling parallel data layout on structured grids
  if (MeshType::dim() == 1)
  {
    // create 1d decomposition
    ierr = DMDACreate1d(mpiCommunicator(), DM_BOUNDARY_NONE, nElementsGlobal_[0], nDofsPerElement, ghostLayerWidth, 
                        NULL, dmElements_.get()); CHKERRV(ierr);
    
    // get global coordinates of local partition
    PetscInt x, m;
    ierr = DMDAGetCorners(*dmElements_, &x, NULL, NULL, &m, NULL, NULL); CHKERRV(ierr);
    beginElementGlobal_[0] = (global_no_t)x;
    nElementsLocal_[0] = (element_no_t)m;
    
    // get number of ranks in each coordinate direction
    ierr = DMDAGetInfo(*dmElements_, NULL, NULL, NULL, NULL, &nRanks_[0], NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL); CHKERRV(ierr);
    
    // get local sizes on the ranks
    const PetscInt *lxData;
    ierr = DMDAGetOwnershipRanges(*dmElements_, &lxData, NULL, NULL);
    
    localSizesOnRanks_[0].assign(lxData,lxData+nRanks_[0]);
  }
  else if (MeshType::dim() == 2)
  {
    // create 2d decomposition
    ierr = DMDACreate2d(mpiCommunicator(), DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DMDA_STENCIL_BOX,
                        nElementsGlobal_[0], nElementsGlobal_[1], PETSC_DECIDE, PETSC_DECIDE,
                        nDofsPerElement, ghostLayerWidth, NULL, NULL, dmElements_.get()); CHKERRV(ierr);
                        
    // get global coordinates of local partition
    PetscInt x, y, m, n;
    ierr = DMDAGetCorners(*dmElements_, &x, &y, NULL, &m, &n, NULL); CHKERRV(ierr);
    beginElementGlobal_[0] = (global_no_t)x;
    beginElementGlobal_[1] = (global_no_t)y;
    nElementsLocal_[0] = (element_no_t)m;
    nElementsLocal_[1] = (element_no_t)n;
    
    // get number of ranks in each coordinate direction
    ierr = DMDAGetInfo(*dmElements_, NULL, NULL, NULL, NULL, &nRanks_[0], &nRanks_[1], NULL, NULL, NULL, NULL, NULL, NULL, NULL); CHKERRV(ierr);
    
    // get local sizes on the ranks
    const PetscInt *lxData;
    const PetscInt *lyData;
    ierr = DMDAGetOwnershipRanges(*dmElements_, &lxData, &lyData, NULL);
    localSizesOnRanks_[0].assign(lxData, lxData + nRanks_[0]);
    localSizesOnRanks_[1].assign(lyData, lyData + nRanks_[1]);
  }
  else if (MeshType::dim() == 3)
  {
    // create 3d decomposition
    ierr = DMDACreate3d(mpiCommunicator(), DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DMDA_STENCIL_BOX,
                        nElementsGlobal_[0], nElementsGlobal_[1], nElementsGlobal_[2],
                        PETSC_DECIDE, PETSC_DECIDE, PETSC_DECIDE,
                        nDofsPerElement, ghostLayerWidth, NULL, NULL, NULL, dmElements_.get()); CHKERRV(ierr);
                        
    // get global coordinates of local partition
    PetscInt x, y, z, m, n, p;
    ierr = DMDAGetCorners(*dmElements_, &x, &y, &z, &m, &n, &p); CHKERRV(ierr);
    beginElementGlobal_[0] = (global_no_t)x;
    beginElementGlobal_[1] = (global_no_t)y;
    beginElementGlobal_[2] = (global_no_t)z;
    nElementsLocal_[0] = (element_no_t)m;
    nElementsLocal_[1] = (element_no_t)n;
    nElementsLocal_[2] = (element_no_t)p;
    
    // get number of ranks in each coordinate direction
    ierr = DMDAGetInfo(*dmElements_, NULL, NULL, NULL, NULL, &nRanks_[0], &nRanks_[1], &nRanks_[2], NULL, NULL, NULL, NULL, NULL, NULL); CHKERRV(ierr);
    
    // get local sizes on the ranks
    const PetscInt *lxData;
    const PetscInt *lyData;
    const PetscInt *lzData;
    ierr = DMDAGetOwnershipRanges(*dmElements_, &lxData, &lyData, &lzData);
    localSizesOnRanks_[0].assign(lxData, lxData + nRanks_[0]);
    localSizesOnRanks_[1].assign(lyData, lyData + nRanks_[1]);
    localSizesOnRanks_[2].assign(lzData, lzData + nRanks_[2]);
  }
  
  initializeHasFullNumberOfNodes();
  setOwnRankPartitioningIndex();

  VLOG(1) << "createDmElements determined the following parameters: "
    << "beginElementGlobal_: " << beginElementGlobal_
    << ", nElementsLocal_: " << nElementsLocal_
    << ", localSizesOnRanks_: " << localSizesOnRanks_
    << ", hasFullNumberOfNodes_: " << hasFullNumberOfNodes_
    << ", ownRankPartitioningIndex/nRanks: " << ownRankPartitioningIndex_ << " / " << nRanks_;
}
  
//! get the local to global mapping for the current partition
template<typename MeshType,typename BasisFunctionType>
ISLocalToGlobalMapping MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
localToGlobalMappingDofs()
{
  return localToGlobalPetscMappingDofs_;
}

template<typename MeshType,typename BasisFunctionType>
int MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nRanks(int coordinateDirection) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());
  return nRanks_[coordinateDirection];
}

//! number of entries in the current partition
template<typename MeshType,typename BasisFunctionType>
element_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nElementsLocal() const
{
  VLOG(1) << "determine nElementsLocal";
  element_no_t result = 1;
  for (int i = 0; i < MeshType::dim(); i++)
  {
    VLOG(1) << "     * " << nElementsLocal_[i];
    result *= nElementsLocal_[i];
  }
  return result;
}

template<typename MeshType,typename BasisFunctionType>
global_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nElementsGlobal() const
{
  global_no_t result = 1;
  for (int i = 0; i < MeshType::dim(); i++)
  {
    result *= nElementsGlobal_[i];
  }
  return result;
}

template<typename MeshType,typename BasisFunctionType>
dof_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nDofsLocalWithGhosts() const
{
  const int nDofsPerNode = FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>::nDofsPerNode();
  
  return nNodesLocalWithGhosts() * nDofsPerNode;
}

template<typename MeshType,typename BasisFunctionType>
dof_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nDofsLocalWithoutGhosts() const
{
  const int nDofsPerNode = FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>::nDofsPerNode();

  return nNodesLocalWithoutGhosts() * nDofsPerNode;
}

template<typename MeshType,typename BasisFunctionType>
global_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nDofsGlobal() const
{
  const int nDofsPerNode = FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>::nDofsPerNode();

  return nNodesGlobal() * nDofsPerNode;
}

//! number of nodes in the local partition
template<typename MeshType,typename BasisFunctionType>
node_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nNodesLocalWithGhosts() const
{
  element_no_t result = 1;
  for (int i = 0; i < MeshType::dim(); i++)
  {
    result *= nNodesLocalWithGhosts(i);
  }
  return result;
}

//! number of nodes in the local partition
template<typename MeshType,typename BasisFunctionType>
node_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nNodesLocalWithoutGhosts() const
{
  element_no_t result = 1;
  for (int i = 0; i < MeshType::dim(); i++)
  {
    result *= nNodesLocalWithoutGhosts(i);
  }
  return result;
}

//! number of nodes in the local partition
template<typename MeshType,typename BasisFunctionType>
node_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nNodesLocalWithGhosts(int coordinateDirection) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());
  
  return this->nElementsLocal_[coordinateDirection] * FunctionSpace::FunctionSpaceBaseDim<1,BasisFunctionType>::averageNNodesPerElement() + 1;
}

//! number of nodes in the local partition
template<typename MeshType,typename BasisFunctionType>
node_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nElementsLocal(int coordinateDirection) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());
  
  return this->nElementsLocal_[coordinateDirection];
}

//! number of nodes in total
template<typename MeshType,typename BasisFunctionType>
node_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nElementsGlobal(int coordinateDirection) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());
  
  return this->nElementsGlobal_[coordinateDirection];
}

//! number of nodes in the local partition
template<typename MeshType,typename BasisFunctionType>
node_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nNodesLocalWithoutGhosts(int coordinateDirection, int partitionIndex) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());

  const int nNodesPer1DElement = FunctionSpace::FunctionSpaceBaseDim<1,BasisFunctionType>::averageNNodesPerElement();

  if (partitionIndex == -1)
  {
    // if partitionIndex was given as -1, it means return the the value for the own partition
    return this->nElementsLocal_[coordinateDirection] * nNodesPer1DElement
      + (this->hasFullNumberOfNodes(coordinateDirection)? 1 : 0);
  }
  else
  {
    // get the value for the given partition with index partitionIndex
    return this->localSizesOnRanks_[coordinateDirection][partitionIndex] * nNodesPer1DElement
      + (this->hasFullNumberOfNodes(coordinateDirection, partitionIndex)? 1 : 0);
  }
}

//! number of nodes in total
template<typename MeshType,typename BasisFunctionType>
global_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nNodesGlobal() const
{
  global_no_t result = 1;
  for (int i = 0; i < MeshType::dim(); i++)
  {
    result *= nNodesGlobal(i);
  }
  return result;
}

template<typename MeshType,typename BasisFunctionType>
global_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
beginNodeGlobalNatural(int coordinateDirection, int partitionIndex) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());
  
  const int nNodesPer1DElement = FunctionSpace::FunctionSpaceBaseDim<1,BasisFunctionType>::averageNNodesPerElement();

  if (partitionIndex == -1)
  {
    // if partitionIndex was given as -1, it means return the global natural no of the beginNode for the current partition
    return beginElementGlobal_[coordinateDirection] * nNodesPer1DElement;
  }
  else
  {
    // compute the global natural no of the beginNode for partition with partitionIndex in coordinateDirection
    global_no_t nodeNoGlobalNatural = 0;
    // loop over all partitions in the given coordinateDirection that are before the partition with partitionIndex
    for (int i = 0; i < partitionIndex; i++)
    {
      // sum up the number of nodes on these previous partitions
      nodeNoGlobalNatural += (localSizesOnRanks_[coordinateDirection][i]) * nNodesPer1DElement;
    }
    return nodeNoGlobalNatural;
  }
}

//! number of nodes in total
template<typename MeshType,typename BasisFunctionType>
global_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
nNodesGlobal(int coordinateDirection) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());
  
  return nElementsGlobal_[coordinateDirection] * FunctionSpace::FunctionSpaceBaseDim<1,BasisFunctionType>::averageNNodesPerElement() + 1; 
}
  
//! get if there are nodes on both borders in the given coordinate direction
//! this is the case if the local partition touches the right/top/back border
template<typename MeshType,typename BasisFunctionType>
bool MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
hasFullNumberOfNodes(int coordinateDirection, int partitionIndex) const
{
  assert(0 <= coordinateDirection);
  assert(coordinateDirection < MeshType::dim());

  if (partitionIndex == -1)
  {
    // if partitionIndex was given as -1, it means return the information for the own partition
    return hasFullNumberOfNodes_[coordinateDirection];
  }
  else
  {
    // determine if the local partition is at the x+/y+/z+ border of the global domain
    return (partitionIndex == nRanks_[coordinateDirection]-1);
  }
}
  
template<typename MeshType,typename BasisFunctionType>
template<typename T>
void MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
extractLocalNodesWithoutGhosts(std::vector<T> &vector, int nComponents) const
{
  LOG(DEBUG) << "extractLocalNodesWithoutGhosts, nComponents=" << nComponents << ", input: " << vector;

  std::vector<T> result(nNodesLocalWithoutGhosts()*nComponents);
  global_no_t resultIndex = 0;
  
  if (MeshType::dim() == 1)
  {
    assert(vector.size() >= (beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0)) * nComponents);
    for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0); i++)
    {
      for (int componentNo = 0; componentNo < nComponents; componentNo++)
      {
        result[resultIndex++] = vector[i*nComponents + componentNo];
      }
    }
  }
  else if (MeshType::dim() == 2)
  {
    assert(vector.size() >= ((beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1) - 1)*nNodesGlobal(0)
      + beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0)) * nComponents);
    for (global_no_t j = beginNodeGlobalNatural(1); j < beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1); j++)
    {
      for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0); i++)
      {
        for (int componentNo = 0; componentNo < nComponents; componentNo++)
        {
          result[resultIndex++] = vector[(j*nNodesGlobal(0) + i)*nComponents + componentNo];
        }
      }
    }
  }
  else if (MeshType::dim() == 3)
  {
    assert(vector.size() >= ((beginNodeGlobalNatural(2) + nNodesLocalWithoutGhosts(2) - 1)*nNodesGlobal(1)*nNodesGlobal(0)
      + (beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1) - 1)*nNodesGlobal(0)
      + beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0)) * nComponents);
    for (global_no_t k = beginNodeGlobalNatural(2); k < beginNodeGlobalNatural(2) + nNodesLocalWithoutGhosts(2); k++)
    {
      for (global_no_t j = beginNodeGlobalNatural(1); j < beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1); j++)
      {
        for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0); i++)
        {
          for (int componentNo = 0; componentNo < nComponents; componentNo++)
          {
            result[resultIndex++] = vector[(k*nNodesGlobal(1)*nNodesGlobal(0) + j*nNodesGlobal(0) + i)*nComponents + componentNo];
          }
        }
      }
    }
  }
  LOG(DEBUG) << "                   output: " << result;
  
  // store values
  vector.assign(result.begin(), result.end());
}
  
template<typename MeshType,typename BasisFunctionType>
void MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
extractLocalDofsWithoutGhosts(std::vector<double> &vector) const 
{
  dof_no_t nDofsPerNode = FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>::nDofsPerNode();
  std::vector<double> result(nDofsLocalWithoutGhosts());
  global_no_t resultIndex = 0;
  
  if (MeshType::dim() == 1)
  {
    if (vector.size() < (beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0))*nDofsPerNode)
    {
      LOG(DEBUG) << "vector.size: " << vector.size() << ", beginNodeGlobalNatural(0): " << beginNodeGlobalNatural(0)
        << ", nNodesLocalWithoutGhosts(0): " << nNodesLocalWithoutGhosts(0)
        << ", nDofsPerNode: " << nDofsPerNode << *this;
    }
    assert(vector.size() >= (beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0))*nDofsPerNode);
    for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0); i++)
    {
      for (int dofOnNodeIndex = 0; dofOnNodeIndex < nDofsPerNode; dofOnNodeIndex++)
      {
        result[resultIndex++] = vector[i*nDofsPerNode + dofOnNodeIndex];
      }
    }
  }
  else if (MeshType::dim() == 2)
  {
    assert(vector.size() >= ((beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1) - 1)*nNodesGlobal(0) + (beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0)))*nDofsPerNode);
    for (global_no_t j = beginNodeGlobalNatural(1); j < beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1); j++)
    {
      for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0); i++)
      {
        for (int dofOnNodeIndex = 0; dofOnNodeIndex < nDofsPerNode; dofOnNodeIndex++)
        {
          result[resultIndex++] = vector[(j*nNodesGlobal(0) + i)*nDofsPerNode + dofOnNodeIndex];
        }
      }
    }
  }
  else if (MeshType::dim() == 3)
  {
    assert(vector.size() >= ((beginNodeGlobalNatural(2) + nNodesLocalWithoutGhosts(2)-1)*nNodesGlobal(1)*nNodesGlobal(0)
      + (beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1)-1)*nNodesGlobal(0) + beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0))*nDofsPerNode);
    for (global_no_t k = beginNodeGlobalNatural(2); k < beginNodeGlobalNatural(2) + nNodesLocalWithoutGhosts(2); k++)
    {
      for (global_no_t j = beginNodeGlobalNatural(1); j < beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1); j++)
      {
        for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0); i++)
        {
          for (int dofOnNodeIndex = 0; dofOnNodeIndex < nDofsPerNode; dofOnNodeIndex++)
          {
            result[resultIndex++] = vector[(k*nNodesGlobal(1)*nNodesGlobal(0) + j*nNodesGlobal(0) + i)*nDofsPerNode + dofOnNodeIndex];
          }
        }
      }
    }
  }
  
  // store values
  vector.assign(result.begin(), result.end());
}

//! get a vector of local dof nos, range [0,nDofsLocalWithoutGhosts] are the dofs without ghost dofs, the whole vector are the dofs with ghost dofs
//! @param onlyNodalValues: if for Hermite only get every second dof such that derivatives are not returned
template<typename MeshType,typename BasisFunctionType>
const std::vector<PetscInt> &MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
dofNosLocal(bool onlyNodalValues) const
{
  if (onlyNodalValues)
  {
    return onlyNodalDofLocalNos_;
  }
  else 
  {
    return this->dofNosLocal_;
  }
}

template<typename MeshType,typename BasisFunctionType>
const std::vector<PetscInt> &MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
ghostDofNosGlobalPetsc() const
{
  return ghostDofNosGlobalPetsc_;
}

//! determine the values of ownRankPartitioningIndex_
template<typename MeshType,typename BasisFunctionType>
void MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
setOwnRankPartitioningIndex()
{
  VLOG(1) << "setOwnRankPartitioningIndex";
  std::array<global_no_t,MeshType::dim()> nodeNoGlobalNatural;

  for (int i = 0; i < MeshType::dim(); i++)
  {
    nodeNoGlobalNatural[i] = beginNodeGlobalNatural(i);
  }

  ownRankPartitioningIndex_ = getPartitioningIndex(nodeNoGlobalNatural);
}


template<typename MeshType,typename BasisFunctionType>
std::array<int,MeshType::dim()> MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
getPartitioningIndex(std::array<global_no_t,MeshType::dim()> nodeNoGlobalNatural)
{
  std::array<int,MeshType::dim()> partitioningIndex;

  const int nNodesPer1DElement = FunctionSpace::FunctionSpaceBaseDim<1,BasisFunctionType>::averageNNodesPerElement();

  // determine the x-index in the partition grid, i.e. how many partitions (other ranks) there are in x direction left of the current ranks' partition
  int partitionX = 0;
  global_no_t xGlobalNatural = 0;
  while (xGlobalNatural <= nodeNoGlobalNatural[0] && xGlobalNatural < nNodesGlobal(0)-1)
  {
    xGlobalNatural += localSizesOnRanks_[0][partitionX++]*nNodesPer1DElement;
    VLOG(1) << "   x GlobalNatural=" << xGlobalNatural << ", partitionX=" << partitionX << ", nodeNoGlobalNatural[0]=" << nodeNoGlobalNatural[0];
  }
  partitionX--;

  // store partitionX
  partitioningIndex[0] = partitionX;

  if (MeshType::dim() >= 2)
  {
    // determine the y-index in the partition grid, i.e. how many partitions (other ranks) there are in y direction before the current ranks' partition
    int partitionY = 0;
    global_no_t yGlobalNatural = 0;
    while (yGlobalNatural <= nodeNoGlobalNatural[1] && yGlobalNatural < nNodesGlobal(1)-1)
    {
      yGlobalNatural += localSizesOnRanks_[1][partitionY++]*nNodesPer1DElement;
      VLOG(1) << "   y GlobalNatural=" << yGlobalNatural << ", partitionY=" << partitionY << ", nodeNoGlobalNatural[1]=" << nodeNoGlobalNatural[1];
    }
    partitionY--;

    // store partitionY
    partitioningIndex[1] = partitionY;
  }

  if (MeshType::dim() == 3)
  {
    // determine the z-index in the partition grid, i.e. how many partitions (other ranks) there are in z direction before the current ranks' partition
    int partitionZ = 0;
    global_no_t zGlobalNatural = 0;
    while (zGlobalNatural <= nodeNoGlobalNatural[2] && zGlobalNatural < nNodesGlobal(2)-1)
    {
      zGlobalNatural += localSizesOnRanks_[2][partitionZ++]*nNodesPer1DElement;
      VLOG(1) << "   z GlobalNatural=" << zGlobalNatural << ", partitionZ=" << partitionZ << ", nodeNoGlobalNatural[2]=" << nodeNoGlobalNatural[2];
    }
    partitionZ--;

    // store partitionZ
    partitioningIndex[2] = partitionZ;
  }

  return partitioningIndex;
}

//! fill the dofLocalNo vectors
template<typename MeshType,typename BasisFunctionType>
void MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
createLocalDofOrderings()
{
  VLOG(1) << "--------------------";
  VLOG(1) << "createLocalDofOrderings " << MeshType::dim() << "D";

  MeshPartitionBase::createLocalDofOrderings(nDofsLocalWithGhosts());
    
  // fill onlyNodalDofLocalNos_
  const int nDofsPerNode = FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>::nDofsPerNode();

  // fill the vector of local dof nos, it contains for each node only the first dof (i.e. not derivatives for Hermite)
  int nNodalDofs = nDofsLocalWithGhosts() / nDofsPerNode;
  onlyNodalDofLocalNos_.resize(nNodalDofs);
  
  dof_no_t localDofNo = 0;
  for (node_no_t localNodeNo = 0; localNodeNo < nNodesLocalWithGhosts(); localNodeNo++)
  {
    onlyNodalDofLocalNos_[localNodeNo] = localDofNo;
    localDofNo += nDofsPerNode;
  }
  
  // fill ghostDofNosGlobalPetsc_
  int resultIndex = 0;
  int nGhostDofs = nDofsLocalWithGhosts() - nDofsLocalWithoutGhosts();
  ghostDofNosGlobalPetsc_.resize(nGhostDofs);
  if (MeshType::dim() == 1)
  {
    VLOG(1) << "ghost nodes range (global natural) x: [" << beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0) << "," << beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0) << "]";
    for (global_no_t i = beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0); i++)
    {
      for (int dofOnNodeIndex = 0; dofOnNodeIndex < nDofsPerNode; dofOnNodeIndex++)
      {
        ghostDofNosGlobalPetsc_[resultIndex++] = i*nDofsPerNode + dofOnNodeIndex;
      }
    }
  }
  else if (MeshType::dim() == 2)
  {
    VLOG(1) << "ghost nodes range (global natural) x: [" << beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0) << "," << beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0) << "]";
    VLOG(1) << "ghost nodes range (global natural) y: [" << beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1) << "," << beginNodeGlobalNatural(1) + nNodesLocalWithGhosts(1) << "]";
    for (global_no_t j = beginNodeGlobalNatural(1); j < beginNodeGlobalNatural(1) + nNodesLocalWithGhosts(1); j++)
    {
      for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0); i++)
      {
        // if node (i,j) is a ghost node
        bool isAtYPlus = (j >= beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1) && j < beginNodeGlobalNatural(1) + nNodesLocalWithGhosts(1));
        bool isAtXPlus = (i >= beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0) && i < beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0));

        if (isAtXPlus || isAtYPlus)
        {

          //  _ _   globalNatural nos in the partitions before current partition
          // |_|_L_    <- (2)
          // |_|_|_|   <- (1)
          // |_|_|_|

          std::array<global_no_t,MeshType::dim()> nodeNoGlobalNatural;
          nodeNoGlobalNatural[0] = i;
          nodeNoGlobalNatural[1] = j;
          std::array<int,MeshType::dim()> partitionIndex = getPartitioningIndex(nodeNoGlobalNatural);

          VLOG(1) << "nodeNoGlobalNatural: " << nodeNoGlobalNatural << ", partitionIndex: " << partitionIndex;
          VLOG(1) << "beginNodeGlobalNatural: " << beginNodeGlobalNatural(0,partitionIndex[0]) << " " << beginNodeGlobalNatural(1,partitionIndex[1]);
          VLOG(1) << "nNodesLocalWithoutGhosts: " << nNodesLocalWithoutGhosts(0,partitionIndex[0]) << " " << nNodesLocalWithoutGhosts(1,partitionIndex[1]);

          // compute globalNatural no from globalPetsc no (i,j)
          global_no_t nodeNoGlobalPetsc = beginNodeGlobalNatural(1,partitionIndex[1])*nNodesGlobal(0)  // (1)
            + beginNodeGlobalNatural(0,partitionIndex[0])*nNodesLocalWithoutGhosts(1,partitionIndex[1])                  // (2)
            + (j-beginNodeGlobalNatural(1,partitionIndex[1])) * nNodesLocalWithoutGhosts(0,partitionIndex[0])
            + (i-beginNodeGlobalNatural(0,partitionIndex[0]));   // in-partition local no

          VLOG(1) << beginNodeGlobalNatural(1,partitionIndex[1]) << "*" << nNodesGlobal(0)
            << "+ " << beginNodeGlobalNatural(0,partitionIndex[0]) << "*" << nNodesLocalWithoutGhosts(1,partitionIndex[1])
            << "+ " << (j-beginNodeGlobalNatural(1,partitionIndex[1])) << "*" << nNodesLocalWithoutGhosts(0,partitionIndex[0])
            << " + " << (i-beginNodeGlobalNatural(0,partitionIndex[0]));

          // loop over dofs of this node and assign global dof nos
          for (int dofOnNodeIndex = 0; dofOnNodeIndex < nDofsPerNode; dofOnNodeIndex++)
          {
            VLOG(1) << "j=" << j << " i=" << i << " nodeNoGlobalPetsc=" << nodeNoGlobalPetsc
              << ", resultIndex=" << resultIndex;
            ghostDofNosGlobalPetsc_[resultIndex++] = nodeNoGlobalPetsc*nDofsPerNode + dofOnNodeIndex;
          }
        }
      }
    }
  }
  else if (MeshType::dim() == 3)
  {
    VLOG(1) << "ghost nodes range (global natural) x: [" << beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0) << "," << beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0) << "]";
    VLOG(1) << "ghost nodes range (global natural) y: [" << beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1) << "," << beginNodeGlobalNatural(1) + nNodesLocalWithGhosts(1) << "]";
    VLOG(1) << "ghost nodes range (global natural) z: [" << beginNodeGlobalNatural(2) + nNodesLocalWithoutGhosts(2) << "," << beginNodeGlobalNatural(2) + nNodesLocalWithGhosts(2) << "]";

    for (global_no_t k = beginNodeGlobalNatural(2); k < beginNodeGlobalNatural(2) + nNodesLocalWithGhosts(2); k++)
    {
      for (global_no_t j = beginNodeGlobalNatural(1); j < beginNodeGlobalNatural(1) + nNodesLocalWithGhosts(1); j++)
      {
        for (global_no_t i = beginNodeGlobalNatural(0); i < beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0); i++)
        {
          bool isAtZPlus = (k >= beginNodeGlobalNatural(2) + nNodesLocalWithoutGhosts(2) && k < beginNodeGlobalNatural(2) + nNodesLocalWithGhosts(2));
          bool isAtYPlus = (j >= beginNodeGlobalNatural(1) + nNodesLocalWithoutGhosts(1) && j < beginNodeGlobalNatural(1) + nNodesLocalWithGhosts(1));
          bool isAtXPlus = (i >= beginNodeGlobalNatural(0) + nNodesLocalWithoutGhosts(0) && i < beginNodeGlobalNatural(0) + nNodesLocalWithGhosts(0));

          // if node (i,j,k) is a ghost node
          if (isAtZPlus || isAtYPlus || isAtXPlus)
          {

            std::array<global_no_t,MeshType::dim()> nodeNoGlobalNatural;

            nodeNoGlobalNatural[0] = i;
            nodeNoGlobalNatural[1] = j;
            nodeNoGlobalNatural[2] = k;
            std::array<int,MeshType::dim()> partitionIndex = getPartitioningIndex(nodeNoGlobalNatural);

            VLOG(1) << "nodeNoGlobalNatural: " << nodeNoGlobalNatural << ", partitionIndex: " << partitionIndex;
            VLOG(1) << "beginNodeGlobalNatural: " << beginNodeGlobalNatural(0,partitionIndex[0]) << " " << beginNodeGlobalNatural(1,partitionIndex[1]) << " " << beginNodeGlobalNatural(2,partitionIndex[2]);
            VLOG(1) << "nNodesLocalWithoutGhosts: " << nNodesLocalWithoutGhosts(0,partitionIndex[0]) << " " << nNodesLocalWithoutGhosts(1,partitionIndex[1]) << " " << nNodesLocalWithoutGhosts(2,partitionIndex[2]);

            // compute globalNatural no from globalPetsc no (i,j,k)
            global_no_t nodeNoGlobalPetsc =
              beginNodeGlobalNatural(2,partitionIndex[2])*nNodesGlobal(1)*nNodesGlobal(0)
              + nNodesLocalWithoutGhosts(2,partitionIndex[2])*beginNodeGlobalNatural(1,partitionIndex[1])*nNodesGlobal(0)  // (1)
              + nNodesLocalWithoutGhosts(2,partitionIndex[2])*nNodesLocalWithoutGhosts(1,partitionIndex[1])*beginNodeGlobalNatural(0,partitionIndex[0])   //(2)
              + (k-beginNodeGlobalNatural(2,partitionIndex[2]))*nNodesLocalWithoutGhosts(1,partitionIndex[1])*nNodesLocalWithoutGhosts(0,partitionIndex[0])
              + (j-beginNodeGlobalNatural(1,partitionIndex[1]))*nNodesLocalWithoutGhosts(0,partitionIndex[0]) + (i-beginNodeGlobalNatural(0,partitionIndex[0]));   // in-partition local no

            for (int dofOnNodeIndex = 0; dofOnNodeIndex < nDofsPerNode; dofOnNodeIndex++)
            {
              VLOG(1) << "k=" << k << " j=" << j << " i=" << i << " nodeNoGlobalPetsc=" << nodeNoGlobalPetsc
                << ", resultIndex=" << resultIndex;
              ghostDofNosGlobalPetsc_[resultIndex++] = nodeNoGlobalPetsc*nDofsPerNode + dofOnNodeIndex;
            }
          }
        }
      }
    }
  }
  
  // create localToGlobalPetscMappingDofs_
  PetscErrorCode ierr;
  Vec temporaryVector;
  ierr = VecCreateGhost(this->mpiCommunicator(), nDofsLocalWithoutGhosts(),
                        nDofsGlobal(), nGhostDofs, ghostDofNosGlobalPetsc_.data(), &temporaryVector); CHKERRV(ierr);
  
  // retrieve local to global mapping
  ierr = VecGetLocalToGlobalMapping(temporaryVector, &localToGlobalPetscMappingDofs_); CHKERRV(ierr);
  //ierr = VecDestroy(&temporaryVector); CHKERRV(ierr);

  VLOG(1) << "VecCreateGhost, nDofsLocalWithoutGhosts: " << nDofsLocalWithoutGhosts() << ", nDofsGlobal: " << nDofsGlobal() << ", ghostDofNosGlobalPetsc_: " << ghostDofNosGlobalPetsc_;
  // VecCreateGhost(MPI_Comm comm,PetscInt n,PetscInt N,PetscInt nghost,const PetscInt ghosts[],Vec *vv)

  VLOG(1) << "n=" << nDofsLocalWithoutGhosts() << ", N=" << nDofsGlobal() << ", nghost=" << nGhostDofs << " ghosts:" << ghostDofNosGlobalPetsc_;
  VLOG(1) << "Result: " << localToGlobalPetscMappingDofs_;
}

template<typename MeshType,typename BasisFunctionType>
global_no_t MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
getElementNoGlobalNatural(element_no_t elementNoLocal) const
{
  if (MeshType::dim() == 1)
  {
    return beginElementGlobal_[0] + elementNoLocal;
  }
  else if (MeshType::dim() == 2)
  {
    element_no_t elementY = element_no_t(elementNoLocal / nElementsLocal(0));
    element_no_t elementX = elementNoLocal % nElementsLocal(0);

    return (beginElementGlobal_[1] + elementY) * nElementsGlobal_[0]
      + beginElementGlobal_[0] + elementX;
  }
  else if (MeshType::dim() == 3)
  {
    element_no_t elementZ = element_no_t(elementNoLocal / (nElementsLocal(0) * nElementsLocal(1)));
    element_no_t elementY = element_no_t((elementNoLocal % (nElementsLocal(0) * nElementsLocal(1))) / nElementsLocal(0));
    element_no_t elementX = elementNoLocal % nElementsLocal(0);

    return (beginElementGlobal_[2] + elementZ) * nElementsGlobal_[0] * nElementsGlobal_[1]
      + (beginElementGlobal_[1] + elementY) * nElementsGlobal_[0]
      + beginElementGlobal_[0] + elementX;
  }
}

template<typename MeshType,typename BasisFunctionType>
void MeshPartition<FunctionSpace::FunctionSpace<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>>::
output(std::ostream &stream)
{
  stream << "MeshPartition<structured>, nElementsGlobal: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << nElementsGlobal_[i] << ",";
  
  stream << " nElementsLocal: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << nElementsLocal_[i] << ",";
  
  stream << " nRanks: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << nRanks_[i] << ",";
  
  stream << " beginElementGlobal: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << beginElementGlobal_[i] << ",";
  
  stream << " hasFullNumberOfNodes: " ;
  for (int i = 0; i < MeshType::dim(); i++)
    stream << hasFullNumberOfNodes_[i] << ",";
  
  stream << " localSizesOnRanks: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << localSizesOnRanks_[i] << ",";
  
  stream << " nNodesGlobal: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << nNodesGlobal(i) << ",";
  stream << "total " << nNodesGlobal() << ",";
  
  stream << " nNodesLocalWithGhosts: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << nNodesLocalWithGhosts(i) << ",";
  stream << "total " << nNodesLocalWithGhosts() << ",";
  
  stream << " nNodesLocalWithoutGhosts: ";
  for (int i = 0; i < MeshType::dim(); i++)
    stream << nNodesLocalWithoutGhosts(i) << ",";
  stream << "total " << nNodesLocalWithoutGhosts()
    << ", dofNosLocal: [";
  for (int i = 0; i < this->dofNosLocal_.size(); i++)
    stream << this->dofNosLocal_[i] << " ";
  stream << "], ghostDofNosGlobalPetsc: [";
    
  for (int i = 0; i < ghostDofNosGlobalPetsc_.size(); i++)
    stream << ghostDofNosGlobalPetsc_[i] << " ";
  stream << "], localToGlobalPetscMappingDofs_: " << localToGlobalPetscMappingDofs_;
} 

}  // namespace
