#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "output_writer/hdf5/hdf5.h"

namespace Checkpointing {
class Generic {
public:
  Generic(DihuContext context, PythonConfig specificSettings);

  bool need_checkpoint();

  template <typename DataType>
  void create_checkpoint(DataType &problemData, int timeStepNo = -1,
                         double currentTime = 0.0) const;
  bool should_exit();

private:
  DihuContext
      context_; //< the context object that holds the config for this class
  PythonConfig specificSettings_; //< config for this object

  mutable OutputWriter::HDF5 writer_;

  int32_t interval_;
  std::string prefix_;
};
} // namespace Checkpointing

#include "checkpointing/generic.tpp"
