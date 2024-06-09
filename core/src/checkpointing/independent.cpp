#include "checkpointing/independent.h"

#include <scr.h>

namespace Checkpointing {
Independent::Independent(DihuContext context,
                         std::shared_ptr<Partition::RankSubset> rankSubset,
                         const std::string &prefix)
    : Generic(), writer_(std::make_unique<OutputWriter::HDF5>(
                     context, PythonConfig(nullptr), rankSubset)),
      prefix_(prefix) {
  if (prefix_ == "") {
    prefix_ = ".";
  }

  writer_->setCombineFiles(false);
  writer_->setWriteMeta(false);
  writer_->setUseCheckpointData(true);
}
} // namespace Checkpointing
