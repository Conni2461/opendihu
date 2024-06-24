#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "output_writer/hdf5/hdf5.h"

namespace Checkpointing {
class Generic {
public:
  Generic(PythonConfig specificSettings);

  bool needCheckpoint();

  template <typename DataType>
  void createCheckpoint(DihuContext context, DataType &problemData,
                        int timeStepNo = -1, double currentTime = 0.0) const;
  bool shouldExit();

  int32_t getInterval() const;
  const char *getPrefix() const;

protected:
  void initWriter(DihuContext context) const;

private:
  PythonConfig specificSettings_; //< config for this object

  mutable std::unique_ptr<OutputWriter::HDF5> writer_;

  int32_t interval_;
  std::string prefix_;
};
} // namespace Checkpointing

#include "checkpointing/generic.tpp"
