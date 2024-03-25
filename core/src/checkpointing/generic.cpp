#include "checkpointing/generic.h"

#include <scr.h>

namespace Checkpointing {
bool Generic::needCheckpoint() {
  int32_t v;
  SCR_Need_checkpoint(&v);
  return v;
}

bool Generic::shouldExit() {
  int32_t v;
  SCR_Should_exit(&v);
  return v;
}
} // namespace Checkpointing
