#include "checkpointing/generic.h"

#include <scr.h>

namespace Checkpointing {
Generic::Generic(PythonConfig specificSettings)
    : specificSettings_(specificSettings), writer_(nullptr),
      interval_(specificSettings.getOptionInt("interval", 1)),
      prefix_(specificSettings.getOptionString("directory", "state")) {}

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

int32_t Generic::getInterval() const { return interval_; }
const char *Generic::getPrefix() const { return prefix_.c_str(); }

void Generic::initWriter(DihuContext context) const {
  writer_ =
      std::make_unique<OutputWriter::HDF5>(context, PythonConfig(nullptr));
  writer_->setCombineFiles(true);
  writer_->setWriteMeta(false);
}
} // namespace Checkpointing
