#include "checkpointing/manager.h"

#include "checkpointing/hdf5/combined.h"
#include "checkpointing/hdf5/independent.h"
#include "checkpointing/json/combined.h"
#include "checkpointing/json/independent.h"

namespace Checkpointing {
Manager::Manager(PythonConfig specificSettings)
    : specificSettings_(specificSettings),
      interval_(specificSettings.getOptionInt("interval", 1)),
      prefix_(specificSettings.getOptionString("directory", "state")),
      type_(specificSettings.getOptionString("type", "hdf5-combined")),
      autoRestore_(specificSettings.getOptionBool("autoRestore", true)),
      checkpointToRestore_(
          specificSettings.getOptionString("checkpointToRestore", "")) {}

void Manager::initialize(DihuContext context,
                         std::shared_ptr<Partition::RankSubset> rankSubset) {
  if (checkpointing) {
    return;
  }
  if (type_ == "hdf5-combined") {
    checkpointing = std::make_shared<HDF5::Combined>(context, rankSubset);
  } else if (type_ == "hdf5-independent") {
    checkpointing =
        std::make_shared<HDF5::Independent>(context, rankSubset, this->prefix_);
  } else if (type_ == "json-combined") {
    checkpointing = std::make_shared<Json::Combined>(context, rankSubset);
  } else if (type_ == "json-independent") {
    checkpointing =
        std::make_shared<Json::Independent>(context, rankSubset, this->prefix_);
  } else {
    LOG(ERROR) << "checkpointing type: " << type_
               << " is not a valid type. Make sure to either configure it "
                  "with the type 'hdf5-combined', 'hdf5-independent', "
                  "'json-combined' or 'json-independent'";
  }
}

bool Manager::needCheckpoint() {
  if (checkpointing) {
    return checkpointing->needCheckpoint();
  }
  return false;
}

bool Manager::shouldExit() {
  if (checkpointing) {
    return checkpointing->shouldExit();
  }
  return false;
}

int32_t Manager::getInterval() const { return interval_; }
const char *Manager::getPrefix() const { return prefix_.c_str(); }
const std::string &Manager::getCheckpointToRestore() const {
  return checkpointToRestore_;
}
} // namespace Checkpointing
