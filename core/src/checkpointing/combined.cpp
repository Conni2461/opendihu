#include "checkpointing/combined.h"

#include <scr.h>

namespace Checkpointing {
Combined::Combined() : Generic(), writer_(nullptr) {}

void Combined::initWriter(DihuContext context) const {
  writer_ =
      std::make_unique<OutputWriter::HDF5>(context, PythonConfig(nullptr));
  writer_->setCombineFiles(true);
  writer_->setWriteMeta(false);
}
} // namespace Checkpointing
