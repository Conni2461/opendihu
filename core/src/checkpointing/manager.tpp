#include "checkpointing/manager.h"

#include "checkpointing/combined.h"

namespace Checkpointing {

template <typename DataType>
void Manager::createCheckpoint(DihuContext context, DataType &problemData,
                               int timeStepNo, double currentTime) const {
  if (std::dynamic_pointer_cast<Combined>(checkpointing) != nullptr) {
    LogScope s("WriteCheckpointCombined");
    std::shared_ptr<Combined> writer =
        std::static_pointer_cast<Combined>(checkpointing);
    writer->createCheckpoint<DataType>(context, problemData, timeStepNo,
                                       currentTime);
  }
}
} // namespace Checkpointing
