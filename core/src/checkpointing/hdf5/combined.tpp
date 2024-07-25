#include "checkpointing/hdf5/combined.h"

#include <scr.h>
#include "input_reader/hdf5/partial.h"
#include "utility/path.h"

namespace Checkpointing {
namespace HDF5 {
template <typename DataType>
void Combined::createCheckpoint(DataType &data, int timeStepNo,
                                double currentTime) const {
  Control::PerformanceMeasurement::start("durationWriteCheckpoint");

  char ckpt_name[SCR_MAX_FILENAME];
  snprintf(ckpt_name, sizeof(ckpt_name), "timestep.%04d", timeStepNo);
  SCR_Start_output(ckpt_name, SCR_FLAG_CHECKPOINT);

  char checkpoint_file[256];
  sprintf(checkpoint_file, "states/timestep.%04d", timeStepNo);

  char scr_file[SCR_MAX_FILENAME];
  SCR_Route_file(checkpoint_file, scr_file);

  writer_->write(data, scr_file, timeStepNo, currentTime);

  SCR_Complete_output(1);

  Control::PerformanceMeasurement::stop("durationWriteCheckpoint");
}

template <typename DataType>
bool Combined::restore(DataType &data, int &timeStepNo, double &currentTime,
                       bool autoRestore,
                       const std::string &checkpointToRestore) const {
  bool restarted = false;
  bool found = false;
  int32_t attempt = 1;
  while (!restarted) {
    if (!autoRestore) {
      break;
    }

    char ckpt_name[SCR_MAX_FILENAME];
    if (checkpointToRestore != "") {
      int32_t r = SCR_Current(checkpointToRestore.c_str());
      if (r != SCR_SUCCESS) {
        LOG(ERROR) << "Failed to specify specific checkpoint. Please check the "
                      "name for typos. Return value"
                   << r;
        return false;
      }
    }

    int32_t have_restart = 0;
    SCR_Have_restart(&have_restart, ckpt_name);
    if (!have_restart) {
      break;
    }

    found = true;
    char checkpointing_dir[SCR_MAX_FILENAME];
    SCR_Start_restart(checkpointing_dir);

    char scr_file[SCR_MAX_FILENAME];
    if (checkpointToRestore != "") {
      SCR_Route_file(checkpointToRestore.c_str(), scr_file);
    } else {
      SCR_Route_file(ckpt_name, scr_file);
    }

    if (!Path::fileExists(scr_file)) {
      break;
    }

    int valid = 1;
    InputReader::HDF5::Partial r(scr_file);

    int32_t step;
    double newTime;
    if (r.hasAttribute("timeStepNo")) {
      step = r.template readAttr<int32_t>("timeStepNo");
    } else {
      break;
    }
    if (r.hasAttribute("currentTime")) {
      newTime = r.template readAttr<double>("currentTime");
    } else {
      break;
    }
    if (!data.restoreState(r)) {
      break;
    }

    LOG(INFO) << "Successfully restored checkpointing timeStepNo: " << step
              << " | currentTime: " << newTime;
    timeStepNo = step;
    currentTime = newTime;

    int rc = SCR_Complete_restart(valid);
    restarted = (rc == SCR_SUCCESS);

    if (!restarted) {
      if (attempt >= this->maxAttempt) {
        LOG(FATAL) << "We were not able to Successfully restore the checkpoint "
                      "on all ranks, only on some. Because of this we might a "
                      "have broken state on some ranks and can't continue "
                      "execution. Aborting application!";
      }

      LOG(ERROR) << "Fatal error, not all ranks were able to restore the "
                    "checkpoint. Trying again. This was attempt: "
                 << attempt << "/" << this->maxAttempt;
      attempt++;
    }
  }

  if (!restarted && found) {
    LOG(ERROR) << "Failed to reload previous state";
    SCR_Complete_restart(0);
  }

  return restarted;
}
} // namespace HDF5
} // namespace Checkpointing
