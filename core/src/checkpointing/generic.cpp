#include "checkpointing/generic.h"

#include <scr.h>

namespace Checkpointing {
Generic::Generic(DihuContext context, PythonConfig specificSettings)
    : context_(context), specificSettings_(specificSettings),
      writer_(context_, PythonConfig()),
      interval_(specificSettings.getOptionInt("interval", 1)),
      prefix_(specificSettings.getOptionString("directory", "state")) {
  writer_.setCombineFiles(true);
  writer_.setWriteMeta(false);
  LOG(ERROR) << "here: " << interval_ << " | " << prefix_;
}

bool Generic::need_checkpoint() {
  int32_t v;
  SCR_Need_checkpoint(&v);
  return v;
}

bool Generic::should_exit() {
  int32_t v;
  SCR_Should_exit(&v);
  return v;
}
} // namespace Checkpointing
