#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "checkpointing/generic.h"
#include "output_writer/hdf5/hdf5.h"

namespace Checkpointing {
class Combined : public Generic {
public:
  Combined(DihuContext context);

  template <typename DataType>
  void createCheckpoint(DataType &problemData, int timeStepNo = -1,
                        double currentTime = 0.0) const;
  template <typename DataType>
  bool restore(DataType &problemData, int &timeStepNo,
               double &currentTime) const;

private:
  std::unique_ptr<OutputWriter::HDF5> writer_;
};
} // namespace Checkpointing

#include "checkpointing/combined.tpp"
