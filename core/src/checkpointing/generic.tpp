#include "checkpointing/generic.h"

#include <scr.h>

namespace Checkpointing {
template <typename DataType>
void Generic::create_checkpoint(DataType &problemData, int timeStepNo,
                                double currentTime) const {
  Control::PerformanceMeasurement::start("durationWriteCheckpoint");

  char ckpt_name[SCR_MAX_FILENAME];
  snprintf(ckpt_name, sizeof(ckpt_name), "timestep.%d", timeStepNo);
  SCR_Start_output(ckpt_name, SCR_FLAG_CHECKPOINT);

  char checkpoint_file[256];
  sprintf(checkpoint_file, "states/timestep.%d", timeStepNo);

  char scr_file[SCR_MAX_FILENAME];
  SCR_Route_file(checkpoint_file, scr_file);

  writer_.write(problemData, scr_file, timeStepNo, currentTime);

  SCR_Complete_output(1);

  Control::PerformanceMeasurement::stop("durationWriteCheckpoint");
}
} // namespace Checkpointing
