#pragma once

#include <set>
#include <memory>

#include "control/types.h"

namespace Partition
{

/** This is a list of ranks that perform a task, e.g. compute a partition.
 */
class RankSubset
{
public:

  //! constructor that constructs a rank subset with all ranks (MPICommWorld)
  RankSubset();

  //! constructor that constructs a rank subset with a single rank
  RankSubset(int singleRank, std::shared_ptr<RankSubset> parentRankSubset = nullptr);
  
  //! constructor that constructs a whole set of ranks which rank nos in terms of the communicator in rankSubset
  //! if rankSubset is nullptr, use MPI_COMM_WORLD
  template<typename Iter>
  RankSubset(Iter ranksBegin, Iter ranksEnd, std::shared_ptr<RankSubset> parentRankSubset = nullptr);
 
  //! number of ranks in the current rank list
  element_no_t size() const;

  //! get the own rank id of the mpi Communicator
  element_no_t ownRankNo();

  //! check if the own rank from MPICommWorld is contained in the current rankSubset
  bool ownRankIsContained() const;

  //! first entry of the rank list
  std::set<int>::const_iterator begin();
  
  //! one after last  entry of the rank list
  std::set<int>::const_iterator end();
  
  //! get the name of the communicator
  std::string communicatorName() const;

  //! get the MPI communicator that contains all ranks of this subset
  MPI_Comm mpiCommunicator() const;
  
  //! increment the number of split communicators from this rank subset by one
  void incrementNCommunicatorSplit();

  //! get the number of times this communicator was split
  int nCommunicatorsSplit() const;

  //! check if the own rank contains the same rank numbers as rankSet
  bool equals(std::set<int> &rankSet) const;

protected:
 
  std::set<int> rankNo_;  ///< the list of ranks
  int ownRankNo_;             ///< own rank id of this rankSubset
  MPI_Comm mpiCommunicator_;    ///< the MPI communicator that contains only the ranks of this rank subset
  std::string communicatorName_;   ///< name of the communicator for debugging output
  int nCommunicatorsSplit_;    ///< the number how often MPI_COMM_SPLIT was called on the current communicator
};

extern int nWorldCommunicatorsSplit;

//! output rank subset
std::ostream &operator<<(std::ostream &stream, RankSubset rankSubset);

}  // namespace

#include "partition/rank_subset.tpp"
