#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "checkpointing/generic.h"
#include "output_writer/hdf5/hdf5.h"

namespace Checkpointing {
namespace HDF5 {
class Independent : public Generic {
public:
  Independent(DihuContext context,
              std::shared_ptr<Partition::RankSubset> rankSubset = nullptr,
              const std::string &prefix = ".");

  template <typename DataType>
  void createCheckpoint(DataType &problemData, int timeStepNo = -1,
                        double currentTime = 0.0) const;
  template <typename DataType>
  bool restore(DataType &problemData, int &timeStepNo, double &currentTime,
               bool autoRestore,
               const std::string &checkpointingToRestore) const;

private:
  std::unique_ptr<OutputWriter::HDF5> writer_;
};
} // namespace HDF5
} // namespace Checkpointing

#include "checkpointing/hdf5/independent.tpp"
