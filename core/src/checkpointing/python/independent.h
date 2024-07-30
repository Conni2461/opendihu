#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "checkpointing/generic.h"
#include "output_writer/python_file/python_file.h"

namespace Checkpointing {
namespace Python {
//! The concreate implementation for Python independent checkpointing
class Independent : public Generic {
public:
  //! Constructs a new independent Python checkpointing implementation
  Independent(DihuContext context,
              std::shared_ptr<Partition::RankSubset> rankSubset = nullptr,
              const std::string &prefix = ".", bool binary = false);

  //! Create a new checkpoint with the problem data as well as the timestep and
  //! the current time.
  template <typename DataType>
  void createCheckpoint(DataType &problemData, int timeStepNo = -1,
                        double currentTime = 0.0) const;
  //! Currently not implemented. Will not restore a checkpoint
  template <typename DataType>
  bool restore(DataType &problemData, int &timeStepNo, double &currentTime,
               bool autoRestore,
               const std::string &checkpointingToRestore) const;

private:
  std::unique_ptr<OutputWriter::PythonFile>
      writer_; //< Constructed output writer that is used to create new
               // checkpoints
};
} // namespace Python
} // namespace Checkpointing

#include "checkpointing/python/independent.tpp"
