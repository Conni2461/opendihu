#include "checkpointing/manager.h"

#include "checkpointing/combined.h"

namespace Checkpointing {
Manager::Manager(PythonConfig specificSettings)
    : specificSettings_(specificSettings),
      interval_(specificSettings.getOptionInt("interval", 1)),
      prefix_(specificSettings.getOptionString("directory", "state")) {}

void Manager::initialize(DihuContext context) {
  if (!checkpointing) {
    checkpointing = std::make_shared<Combined>(context);
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
} // namespace Checkpointing
