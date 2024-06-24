#include "checkpointing/manager.h"

#include "checkpointing/combined.h"

namespace Checkpointing {
template <typename DataType>
void Manager::createCheckpoint(DataType &problemData, int timeStepNo,
                               double currentTime) const {
  if (std::dynamic_pointer_cast<Combined>(checkpointing) != nullptr) {
    LogScope s("WriteCheckpointCombined");
    std::shared_ptr<Combined> writer =
        std::static_pointer_cast<Combined>(checkpointing);
    writer->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  }
}

template <typename DataType>
bool Manager::restore(DataType &problemData, int &timeStepNo,
                      double &currentTime) const {
  if (std::dynamic_pointer_cast<Combined>(checkpointing) != nullptr) {
    LogScope s("RestoreCheckpointCombined");
    std::shared_ptr<Combined> writer =
        std::static_pointer_cast<Combined>(checkpointing);
    return writer->restore<DataType>(problemData, timeStepNo, currentTime);
  }
  return false;
}
} // namespace Checkpointing
