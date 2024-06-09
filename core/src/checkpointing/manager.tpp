#include "checkpointing/manager.h"

#include "checkpointing/combined.h"
#include "checkpointing/independent.h"

namespace Checkpointing {
template <typename DataType>
void Manager::createCheckpoint(DataType &problemData, int timeStepNo,
                               double currentTime) const {
  if (std::dynamic_pointer_cast<Combined>(checkpointing) != nullptr) {
    LogScope s("WriteCheckpointCombined");
    std::shared_ptr<Combined> obj =
        std::static_pointer_cast<Combined>(checkpointing);
    obj->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  } else if (std::dynamic_pointer_cast<Independent>(checkpointing) != nullptr) {
    LogScope s("WriteCheckpointIndependent");
    std::shared_ptr<Independent> obj =
        std::static_pointer_cast<Independent>(checkpointing);
    obj->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  }
}

template <typename DataType>
bool Manager::restore(DataType &problemData, int &timeStepNo,
                      double &currentTime) const {
  std::stringstream ss;
  if (this->checkpointToRestore_ != "") {
    ss << this->prefix_ << "/" << this->checkpointToRestore_;
  }
  if (std::dynamic_pointer_cast<Combined>(checkpointing) != nullptr) {
    LogScope s("RestoreCheckpointCombined");
    std::shared_ptr<Combined> obj =
        std::static_pointer_cast<Combined>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  } else if (std::dynamic_pointer_cast<Independent>(checkpointing) != nullptr) {
    LogScope s("RestoreCheckpointIndependent");
    std::shared_ptr<Independent> obj =
        std::static_pointer_cast<Independent>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  }
  return false;
}
} // namespace Checkpointing
