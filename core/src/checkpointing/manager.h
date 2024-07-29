#pragma once

#include <Python.h> // has to be the first included header
#include <memory>

#include "control/types.h"
#include "checkpointing/generic.h"

namespace Checkpointing {
class Handle;

class Manager {
public:
  Manager(PythonConfig specificSettings);

  std::shared_ptr<Handle>
  initialize(DihuContext context,
             std::shared_ptr<Partition::RankSubset> rankSubset = nullptr);

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
};

class Handle {
public:
  Handle(std::shared_ptr<Generic> checkpointing, bool autoRestore,
         const std::string &checkpointToRestore, const std::string &prefix);

  template <typename DataType>
  void createCheckpoint(DataType &problemData, int timeStepNo = -1,
                        double currentTime = 0.0) const;

  template <typename DataType>
  bool restore(DataType &problemData, int &timeStepNo,
               double &currentTime) const;

  bool needCheckpoint();
  bool shouldExit();

private:
  std::shared_ptr<Generic> checkpointing_;
  std::string prefix_;
  bool autoRestore_; //! if set it restores if there is a checkpoint found
  std::string checkpointToRestore_;
};

} // namespace Checkpointing

#include "checkpointing/manager.tpp"
