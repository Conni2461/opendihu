#include "checkpointing/manager.h"

#include "checkpointing/hdf5/combined.h"
#include "checkpointing/hdf5/independent.h"

namespace Checkpointing {
template <typename DataType>
void Manager::createCheckpoint(DataType &problemData, int timeStepNo,
                               double currentTime) const {
  if (std::dynamic_pointer_cast<HDF5::Combined>(checkpointing) != nullptr) {
    LogScope s("WriteCheckpointCombined");
    std::shared_ptr<HDF5::Combined> obj =
        std::static_pointer_cast<HDF5::Combined>(checkpointing);
    obj->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  } else if (std::dynamic_pointer_cast<HDF5::Independent>(checkpointing) !=
             nullptr) {
    LogScope s("WriteCheckpointIndependent");
    std::shared_ptr<HDF5::Independent> obj =
        std::static_pointer_cast<HDF5::Independent>(checkpointing);
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
  if (std::dynamic_pointer_cast<HDF5::Combined>(checkpointing) != nullptr) {
    LogScope s("RestoreCheckpointCombined");
    std::shared_ptr<HDF5::Combined> obj =
        std::static_pointer_cast<HDF5::Combined>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  } else if (std::dynamic_pointer_cast<HDF5::Independent>(checkpointing) !=
             nullptr) {
    LogScope s("RestoreCheckpointIndependent");
    std::shared_ptr<HDF5::Independent> obj =
        std::static_pointer_cast<HDF5::Independent>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  }
  return false;
}
} // namespace Checkpointing
