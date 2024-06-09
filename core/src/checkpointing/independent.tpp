#include "checkpointing/independent.h"

#include <scr.h>
#include "input_reader/hdf5.h"
#include "utility/path.h"

namespace Checkpointing {
template <typename DataType>
void Independent::createCheckpoint(DataType &data, int timeStepNo,
                                   double currentTime) const {
  Control::PerformanceMeasurement::start("durationWriteCheckpoint");

  int32_t ownRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &ownRank);

  std::stringstream ckptName;
  ckptName << "timestep." << timeStepNo << "d";
  SCR_Start_output(ckptName.str().c_str(), SCR_FLAG_CHECKPOINT);

  std::stringstream checkpointDirStream;
  checkpointDirStream << prefix_ << "/timestep." << timeStepNo << "d";
  std::string checkpointDir = checkpointDirStream.str();

  if (ownRank == 0) {
    Path::mkpath(checkpointDir.c_str());
  }

  // hold all processes until directory is created
  MPI_Barrier(MPI_COMM_WORLD);

  std::stringstream checkpointFile;
  checkpointFile << checkpointDir << "/rank_" << ownRank << ".ckpt";

  char scr_file[SCR_MAX_FILENAME];
  SCR_Route_file(checkpointFile.str().c_str(), scr_file);

  writer_->write(data, scr_file, timeStepNo, currentTime);

  SCR_Complete_output(1);

  Control::PerformanceMeasurement::stop("durationWriteCheckpoint");
}

template <typename DataType>
bool Independent::restore(DataType &data, int &timeStepNo, double &currentTime,
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
} // namespace Checkpointing
