#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"
#include "checkpointing/generic.h"
#include "output_writer/python_file/python_file.h"

namespace Checkpointing {
namespace Python {
class Independent : public Generic {
public:
  Independent(DihuContext context,
              std::shared_ptr<Partition::RankSubset> rankSubset = nullptr,
              const std::string &prefix = ".", bool binary = false);

  template <typename DataType>
  void createCheckpoint(DataType &problemData, int timeStepNo = -1,
                        double currentTime = 0.0) const;
  template <typename DataType>
  bool restore(DataType &problemData, int &timeStepNo, double &currentTime,
               bool autoRestore,
               const std::string &checkpointingToRestore) const;

private:
  std::unique_ptr<OutputWriter::PythonFile> writer_;
};
} // namespace Python
} // namespace Checkpointing

#include "checkpointing/python/independent.tpp"
