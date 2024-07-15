#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "checkpointing/generic.h"
#include "output_writer/paraview/paraview.h"

namespace Checkpointing {
namespace Paraview {
class Combined : public Generic {
public:
  Combined(DihuContext context,
           std::shared_ptr<Partition::RankSubset> rankSubset = nullptr);

  template <typename DataType>
  void createCheckpoint(DataType &problemData, int timeStepNo = -1,
                        double currentTime = 0.0) const;
  template <typename DataType>
  bool restore(DataType &problemData, int &timeStepNo, double &currentTime,
               bool autoRestore,
               const std::string &checkpointingToRestore) const;

private:
  std::unique_ptr<OutputWriter::Paraview> writer_;
};
} // namespace Paraview
} // namespace Checkpointing

#include "checkpointing/paraview/combined.tpp"
