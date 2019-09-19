#include "partition/rank_subset.h"

#include <numeric>
#include <algorithm>

#include "easylogging++.h"
#include "utility/mpi_utility.h"
#include "utility/vector_operators.h"
#include "control/dihu_context.h"

namespace Partition 
{
  
int nWorldCommunicatorsSplit = 0;

RankSubset::RankSubset() : ownRankNo_(-1)
{
  VLOG(1) << "RankSubset empty constructor";

  // create copy MPI_COMM_WORLD
  MPIUtility::handleReturnValue(MPI_Comm_dup(MPI_COMM_WORLD, &mpiCommunicator_), "MPI_Comm_dup");
  nWorldCommunicatorsSplit++;
  nCommunicatorsSplit_ = 0;
  isWorldCommunicator_ = true;

  if (mpiCommunicator_ == MPI_COMM_NULL)
  {
    LOG(FATAL) << "Failed to duplicate MPI_COMM_WORLD";
  }

  communicatorName_ = "COMM_WORLD";
  MPIUtility::handleReturnValue(MPI_Comm_set_name(mpiCommunicator_, communicatorName_.c_str()), "MPI_Comm_set_name");

  // get number of ranks
  int nRanks;
  MPIUtility::handleReturnValue(MPI_Comm_size(mpiCommunicator_, &nRanks), "MPI_Comm_size");
  
  // create list of all ranks
  for (int i = 0; i < nRanks; i++)
  {
    rankNo_.insert(i);
  }
  VLOG(1) << "initialized as COMM_WORLD: " << rankNo_;
}
  
RankSubset::RankSubset(int singleRank, std::shared_ptr<RankSubset> parentRankSubset) : ownRankNo_(-1), nCommunicatorsSplit_(0)
{
  rankNo_.clear();
  rankNo_.insert(singleRank);

  MPI_Comm parentCommunicator = MPI_COMM_WORLD;
  int ownRankParentCommunicator = 0;
  isWorldCommunicator_ = false;

  // if a parent rank subset was given, use it
  if (parentRankSubset)
  {
    parentCommunicator = parentRankSubset->mpiCommunicator();
    ownRankParentCommunicator = parentRankSubset->ownRankNo();
  }
  else
  {
    // get the own current MPI rank from the MPI_COMM_WORLD
    MPIUtility::handleReturnValue(MPI_Comm_rank(parentCommunicator, &ownRankParentCommunicator), "MPI_Comm_rank");
  }
  int color = MPI_UNDEFINED;

  LOG(DEBUG) << "ownRankParentCommunicator: " << ownRankParentCommunicator << ", singleRank for which to create RankSubset: " << singleRank;

  // if ownRankParentCommunicator is contained in rank subset
  if (singleRank == ownRankParentCommunicator)
    color = singleRank;

  // create new communicator which contains all ranks that have the same value of color (and not MPI_UNDEFINED)
  MPIUtility::handleReturnValue(MPI_Comm_split(parentCommunicator, color, 0, &mpiCommunicator_), "MPI_Comm_split");

  // all ranks that are not part of the communicator will store "MPI_COMM_NULL" as mpiCommunicator_
#if 1
  // assign the name of the new communicator
  if (ownRankIsContained())
  {
    // get name of old communicator
    VLOG(1) << "MPI_MAX_OBJECT_NAME: " << MPI_MAX_OBJECT_NAME;

    std::vector<char> oldCommunicatorNameStr(MPI_MAX_OBJECT_NAME);
    int oldCommunicatorNameLength = 0;
    MPIUtility::handleReturnValue(MPI_Comm_get_name(parentCommunicator, oldCommunicatorNameStr.data(), &oldCommunicatorNameLength), "MPI_Comm_get_name");

    std::string oldCommunicatorName(oldCommunicatorNameStr.begin(), oldCommunicatorNameStr.begin()+oldCommunicatorNameLength);
    VLOG(1) << "oldCommunicatorName: " << oldCommunicatorName;

    // define new name
    std::stringstream newCommunicatorName;
    if (parentRankSubset)
    {
      newCommunicatorName << oldCommunicatorName << "_" << parentRankSubset->nCommunicatorsSplit();
    }
    else
    {
      newCommunicatorName << oldCommunicatorName << "_" << nWorldCommunicatorsSplit;
    }
    VLOG(1) << "newCommunicatorName: " << newCommunicatorName.str();

    // assign name
    communicatorName_ = newCommunicatorName.str();
    MPIUtility::handleReturnValue(MPI_Comm_set_name(mpiCommunicator_, communicatorName_.c_str()), "MPI_Comm_set_name");
  }

  if (parentRankSubset)
  {
    parentRankSubset->incrementNCommunicatorSplit();
  }
  else
  {
    nWorldCommunicatorsSplit++;
  }

#endif
  // get number of ranks
#if 0
  if (ownRankIsContained())
  {
    int nRanks;
    MPIUtility::handleReturnValue(MPI_Comm_size(mpiCommunicator_, &nRanks), "MPI_Comm_size");
    if (nRanks != 1)
    {
      LOG(DEBUG) << "nRanks: " << nRanks;
    }
    assert(nRanks == 1);
  }
#endif
}

std::set<int>::const_iterator RankSubset::begin()
{
  return rankNo_.cbegin();
}

std::set<int>::const_iterator RankSubset::end()
{
  return rankNo_.cend();
}

element_no_t RankSubset::size() const
{
  return rankNo_.size();
}

void RankSubset::incrementNCommunicatorSplit()
{
  nCommunicatorsSplit_++;
}

int RankSubset::nCommunicatorsSplit() const
{
  return nCommunicatorsSplit_;
}

bool RankSubset::ownRankIsContained() const
{
  // all ranks that are not part of the communicator will store "MPI_COMM_NULL" as mpiCommunicator_
  return mpiCommunicator_ != MPI_COMM_NULL;
}

element_no_t RankSubset::ownRankNo()
{
  if (ownRankNo_ == -1 && mpiCommunicator_ != MPI_COMM_NULL)
  {
    // get the own rank id in this communicator
    MPIUtility::handleReturnValue(MPI_Comm_rank(mpiCommunicator_, &ownRankNo_), "MPI_Comm_rank");
  }
  return ownRankNo_;
}

MPI_Comm RankSubset::mpiCommunicator() const
{
  if (mpiCommunicator_ == MPI_COMM_NULL)
  {
    LOG(ERROR) << "Accessing NULL communicator";
  }
  return mpiCommunicator_;
}

std::string RankSubset::communicatorName() const
{
  return communicatorName_;
}
  
bool RankSubset::equals(std::set<int> &rankSet) const
{
  return rankSet == rankNo_;
}

std::ostream &operator<<(std::ostream &stream, RankSubset rankSubset)
{
  if (rankSubset.size() == 0)
  {
    stream << "(empty rankSubset)";
  }
  else 
  {
    stream << "(" << rankSubset.communicatorName() << ": ";
    
    for (std::set<int>::const_iterator iterRank = rankSubset.begin(); iterRank != rankSubset.end(); iterRank++)
    {
      if (iterRank != rankSubset.begin())
        stream << ", ";
      if (rankSubset.ownRankNo() == *iterRank)
        stream << "*";
      stream << *iterRank;
    }
    stream << ")";
  }
  return stream;
}

}  // namespace
