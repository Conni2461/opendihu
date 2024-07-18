#include "checkpointing/python/independent.h"

#include <scr.h>
#include "utility/path.h"

namespace Checkpointing {
namespace Python {
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
  checkpointDirStream << prefix_ << "/timestep." << std::setw(4)
                      << std::setfill('0') << timeStepNo << "d";
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

  writer_->write(data, timeStepNo, currentTime, 1, scr_file);

  SCR_Complete_output(1);

  Control::PerformanceMeasurement::stop("durationWriteCheckpoint");
}

template <typename DataType>
bool Independent::restore(DataType &data, int &timeStepNo, double &currentTime,
                          bool autoRestore,
                          const std::string &checkpointToRestore) const {
  int32_t ownRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &ownRank);

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
} // namespace Python
} // namespace Checkpointing
