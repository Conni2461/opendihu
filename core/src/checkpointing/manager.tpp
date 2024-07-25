#include "checkpointing/manager.h"

#include "checkpointing/hdf5/combined.h"
#include "checkpointing/hdf5/independent.h"
#include "checkpointing/json/combined.h"
#include "checkpointing/json/independent.h"

namespace Checkpointing {
template <typename DataType>
void Manager::createCheckpoint(DataType &problemData, int timeStepNo,
                               double currentTime) const {
  if (std::dynamic_pointer_cast<HDF5::Combined>(checkpointing) != nullptr) {
    LogScope s("WriteCheckpointHDF5Combined");
    std::shared_ptr<HDF5::Combined> obj =
        std::static_pointer_cast<HDF5::Combined>(checkpointing);
    obj->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  } else if (std::dynamic_pointer_cast<HDF5::Independent>(checkpointing) !=
             nullptr) {
    LogScope s("WriteCheckpointHDF5Independent");
    std::shared_ptr<HDF5::Independent> obj =
        std::static_pointer_cast<HDF5::Independent>(checkpointing);
    obj->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  } else if (std::dynamic_pointer_cast<Json::Combined>(checkpointing) !=
             nullptr) {
    LogScope s("WriteCheckpointJsonCombined");
    std::shared_ptr<Json::Combined> obj =
        std::static_pointer_cast<Json::Combined>(checkpointing);
    obj->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  } else if (std::dynamic_pointer_cast<Json::Independent>(checkpointing) !=
             nullptr) {
    LogScope s("WriteCheckpointJsonIndependent");
    std::shared_ptr<Json::Independent> obj =
        std::static_pointer_cast<Json::Independent>(checkpointing);
    obj->createCheckpoint<DataType>(problemData, timeStepNo, currentTime);
  }
}

template <typename DataType>
bool Manager::restore(DataType &problemData, int &timeStepNo,
                      double &currentTime) const {
  std::stringstream ss;
  if (this->checkpointToRestore_ != "") {
    ss << this->checkpointToRestore_;
  }
  if (std::dynamic_pointer_cast<HDF5::Combined>(checkpointing) != nullptr) {
    LogScope s("RestoreCheckpointHDF5Combined");
    std::shared_ptr<HDF5::Combined> obj =
        std::static_pointer_cast<HDF5::Combined>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  } else if (std::dynamic_pointer_cast<HDF5::Independent>(checkpointing) !=
             nullptr) {
    LogScope s("RestoreCheckpointHDF5Independent");
    std::shared_ptr<HDF5::Independent> obj =
        std::static_pointer_cast<HDF5::Independent>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  } else if (std::dynamic_pointer_cast<Json::Combined>(checkpointing) !=
             nullptr) {
    LogScope s("RestoreCheckpointJsonCombined");
    std::shared_ptr<Json::Combined> obj =
        std::static_pointer_cast<Json::Combined>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  } else if (std::dynamic_pointer_cast<Json::Independent>(checkpointing) !=
             nullptr) {
    LogScope s("RestoreCheckpointJsonIndependent");
    std::shared_ptr<Json::Independent> obj =
        std::static_pointer_cast<Json::Independent>(checkpointing);
    return obj->restore<DataType>(problemData, timeStepNo, currentTime,
                                  this->autoRestore_, ss.str());
  }
  return false;
}
} // namespace Checkpointing
