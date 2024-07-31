#pragma once

#include "control/types.h"

namespace InputReader {
//! Base input reader class, that every input reader needs to implement. It
//! provides two function for reading data into output variables.
class Generic {
public:
  //! Default constructor for a base input reader
  Generic() = default;
  //! Default destructor for a base input reader
  virtual ~Generic() = default;

  //! Read a dataset of integers in a flat vector passed in using an out
  //! variable
  virtual bool readIntVector(const char *name, std::vector<int32_t> &out,
                             const std::string &groupName = "") const = 0;
  //! Read a dataset of doubles in a flat vector passed in using an out variable
  virtual bool readDoubleVector(const char *name, std::vector<double> &out,
                                const std::string &groupName = "") const = 0;

  //! Wrapper around readDoubleVector that reads maxSize items into a vector of
  //! D dimensional arrays.
  //! If the maxSize is set to -1, we will read the full vector into D
  //! dimensions, otherwise maxSize % D == 0 is expected and this function will
  //! return fales if this is not given.
  template <int D>
  bool readDoubleVecD(const char *name, ssize_t maxSize,
                      std::vector<VecD<D>> &out,
                      const std::string &groupName = "") const;
};
} // namespace InputReader

#include "input_reader/generic.tpp"
