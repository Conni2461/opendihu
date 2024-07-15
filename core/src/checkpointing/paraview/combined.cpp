#include "checkpointing/paraview/combined.h"

namespace Checkpointing {
namespace Paraview {
Combined::Combined(DihuContext context,
                   std::shared_ptr<Partition::RankSubset> rankSubset)
    : Generic(), writer_(std::make_unique<OutputWriter::Paraview>(
                     context, PythonConfig(nullptr), rankSubset)) {
  writer_->setCombineFiles(true);

  // TODO: would be nice but we can emit this for now
  // writer_->setWriteMeta(false);
  // writer_->setUseCheckpointData(true);
}
} // namespace Paraview
} // namespace Checkpointing
