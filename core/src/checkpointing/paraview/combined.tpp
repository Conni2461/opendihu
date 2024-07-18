#include "checkpointing/paraview/combined.h"

#include <scr.h>
#include "utility/path.h"

namespace Checkpointing {
namespace Paraview {
template <typename DataType>
void Combined::createCheckpoint(DataType &data, int timeStepNo,
                                double currentTime) const {
  Control::PerformanceMeasurement::start("durationWriteCheckpoint");

  char ckpt_name[SCR_MAX_FILENAME];
  snprintf(ckpt_name, sizeof(ckpt_name), "timestep.%d", timeStepNo);
  SCR_Start_output(ckpt_name, SCR_FLAG_CHECKPOINT);

  char checkpoint_file[256];
  sprintf(checkpoint_file, "states/timestep.%04d", timeStepNo);

  char scr_file[SCR_MAX_FILENAME];
  SCR_Route_file(checkpoint_file, scr_file);

  writer_->write(data, timeStepNo, currentTime, 1, scr_file);

  SCR_Complete_output(1);

  Control::PerformanceMeasurement::stop("durationWriteCheckpoint");
}

template <typename DataType>
bool Combined::restore(DataType &data, int &timeStepNo, double &currentTime,
                       bool autoRestore,
                       const std::string &checkpointToRestore) const {
  bool restarted = false;
  bool found = false;
  while (!restarted) {
    if (!autoRestore) {
      break;
    }
  }

  if (!restarted && found) {
    LOG(ERROR) << "Failed to reload previous state";
    SCR_Complete_restart(0);
  }

  return restarted;
}
} // namespace Paraview
} // namespace Checkpointing
