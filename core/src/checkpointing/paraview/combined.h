#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "checkpointing/generic.h"
#include "output_writer/paraview/paraview.h"

namespace Checkpointing {
namespace Paraview {
//! The concreate implementation for Paraview combined checkpointing
class Combined : public Generic {
public:
  //! Constructs a new combined Paraview checkpointing implementation
  Combined(DihuContext context,
           std::shared_ptr<Partition::RankSubset> rankSubset = nullptr,
           const std::string &prefix = ".");

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
  std::unique_ptr<OutputWriter::Paraview>
      writer_; //< Constructed output writer that is used to create new
               //checkpoints
};
} // namespace Paraview
} // namespace Checkpointing

#include "checkpointing/paraview/combined.tpp"
