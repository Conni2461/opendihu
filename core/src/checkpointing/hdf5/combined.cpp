#include "checkpointing/hdf5/combined.h"

#include <scr.h>

namespace Checkpointing {
namespace HDF5 {
Combined::Combined(DihuContext context,
                   std::shared_ptr<Partition::RankSubset> rankSubset)
    : Generic(), writer_(std::make_unique<OutputWriter::HDF5>(
                     context, PythonConfig(nullptr), rankSubset)) {
  writer_->setCombineFiles(true);
  writer_->setWriteMeta(false);
  writer_->setUseCheckpointData(true);
}
} // namespace HDF5
} // namespace Checkpointing
