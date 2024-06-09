#pragma once

#include <Python.h> // has to be the first included header
#include <memory>

#include "control/types.h"
#include "checkpointing/generic.h"

namespace Checkpointing {
class Manager {
public:
  Manager(PythonConfig specificSettings);

  void initialize(DihuContext context);

  template <typename DataType>
  void createCheckpoint(DataType &problemData, int timeStepNo = -1,
                        double currentTime = 0.0) const;

  template <typename DataType>
  bool restore(DataType &problemData, int &timeStepNo,
               double &currentTime) const;

  bool needCheckpoint();
  bool shouldExit();

  int32_t getInterval() const;
  const char *getPrefix() const;
  const std::string &getCheckpointToRestore() const;

private:
  PythonConfig specificSettings_; //< config for this object

  int32_t interval_;
  std::string prefix_;
  std::string type_;
  bool autoRestore_; //! if set it restores if there is a checkpoint found
  std::string checkpointToRestore_;

  std::shared_ptr<Generic> checkpointing;
};
} // namespace Checkpointing

#include "checkpointing/manager.tpp"
