#include "checkpointing/combined.h"

#include <scr.h>
#include "input_reader/hdf5.h"

namespace Checkpointing {
template <typename DataType>
void Combined::createCheckpoint(DataType &data, int timeStepNo,
                                double currentTime) const {
  Control::PerformanceMeasurement::start("durationWriteCheckpoint");

  char ckpt_name[SCR_MAX_FILENAME];
  snprintf(ckpt_name, sizeof(ckpt_name), "timestep.%d", timeStepNo);
  SCR_Start_output(ckpt_name, SCR_FLAG_CHECKPOINT);

  char checkpoint_file[256];
  sprintf(checkpoint_file, "states/timestep.%d", timeStepNo);

  char scr_file[SCR_MAX_FILENAME];
  SCR_Route_file(checkpoint_file, scr_file);

  writer_->write(data, scr_file, timeStepNo, currentTime);

  SCR_Complete_output(1);

  Control::PerformanceMeasurement::stop("durationWriteCheckpoint");
}

template <typename DataType>
bool Combined::restore(DataType &data, int &timeStepNo,
                       double &currentTime) const {
  bool restarted = false;
  bool found = false;
  while (!restarted) {
    int32_t have_restart = 0;
    char ckpt_name[SCR_MAX_FILENAME];
    SCR_Have_restart(&have_restart, ckpt_name);
    if (!have_restart) {
      break;
    }

    found = true;
    char checkpointing_dir[SCR_MAX_FILENAME];
    SCR_Start_restart(checkpointing_dir);

    char scr_file[SCR_MAX_FILENAME];
    SCR_Route_file(ckpt_name, scr_file);

    int valid = 1;
    InputReader::HDF5 r(scr_file);

    int32_t step;
    double newTime;
    if (r.hasAttribute("timeStepNo")) {
      step = r.readInt32Attribute("timeStepNo");
    } else {
      break;
    }
    if (r.hasAttribute("currentTime")) {
      newTime = r.readDoubleAttribute("currentTime");
    } else {
      break;
    }
    if (!data.restoreState(r)) {
      break;
    }

    LOG(DEBUG) << "Successfully restored checkpointing timeStepNo: " << step
               << " | currentTime: " << newTime;
    timeStepNo = step;
    currentTime = newTime;

    int rc = SCR_Complete_restart(valid);
    restarted = (rc == SCR_SUCCESS);
  }

  if (!restarted && found) {
    LOG(ERROR) << "Failed to reload previous state";
    SCR_Complete_restart(0);
  }

  return restarted;
}
} // namespace Checkpointing
