#include "checkpointing/hdf5/combined.h"

namespace Checkpointing {
namespace HDF5 {
Combined::Combined(DihuContext context,
                   std::shared_ptr<Partition::RankSubset> rankSubset,
                   const std::string &prefix, bool async)
    : Generic(prefix), writer_(std::make_unique<OutputWriter::HDF5>(
                           context, PythonConfig(nullptr), rankSubset)) {
  writer_->setCombineFiles(true);
  writer_->setWriteMeta(true);
  writer_->setUseCheckpointData(true);
  writer_->setAsync(async);
}
} // namespace HDF5
} // namespace Checkpointing
