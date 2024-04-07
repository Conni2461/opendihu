#include "checkpointing/combined.h"

#include <scr.h>

namespace Checkpointing {
Combined::Combined(DihuContext context)
    : Generic(), writer_(std::make_unique<OutputWriter::HDF5>(
                     context, PythonConfig(nullptr))) {
  writer_->setCombineFiles(true);
  writer_->setWriteMeta(false);
  writer_->setUseCheckpointData(true);
}
} // namespace Checkpointing
