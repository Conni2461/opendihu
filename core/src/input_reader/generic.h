#pragma once

#include "control/types.h"

namespace InputReader {
class Generic {
public:
  Generic() = default;
  virtual ~Generic() = default;

  //! Read a dataset of integers in a flat vector passed in using an out
  //! variable
  virtual bool readIntVector(const char *name,
                             std::vector<int32_t> &out) const = 0;
  //! Read a dataset of doubles in a flat vector passed in using an out variable
  virtual bool readDoubleVector(const char *name,
                                std::vector<double> &out) const = 0;
};
} // namespace InputReader
