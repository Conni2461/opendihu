#include "checkpointing/manager.h"

#include "checkpointing/combined.h"
#include "checkpointing/independent.h"

namespace Checkpointing {
Manager::Manager(PythonConfig specificSettings)
    : specificSettings_(specificSettings),
      interval_(specificSettings.getOptionInt("interval", 1)),
      prefix_(specificSettings.getOptionString("directory", "state")),
      type_(specificSettings.getOptionString("type", "combined")),
      autoRestore_(specificSettings.getOptionBool("autoRestore", true)),
      checkpointToRestore_(
          specificSettings.getOptionString("checkpointToRestore", "")) {}

void Manager::initialize(DihuContext context) {
  if (!checkpointing) {
    if (type_ == "combined") {
      checkpointing = std::make_shared<Combined>(context, context.rankSubset());
    } else if (type_ == "independent") {
      checkpointing = std::make_shared<Independent>(
          context, context.rankSubset(), this->prefix_);
    } else {
      LOG(ERROR) << "checkpointing type: " << type_
                 << " is not a valid type. Make sure to either configure it "
                    "with the type 'combined' or 'independent'";
    }
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
