#include "control/diagnostic_tool/timing_measurement.h"

#include <stdlib.h>

namespace Control {
std::map<int32_t, TimingMeasurement::Measurement>
    TimingMeasurement::measurements_;

void TimingMeasurement::start(int32_t timestep) {
  auto iter = measurements_.find(timestep);

  if (iter == measurements_.end()) {
    auto insertedIter = measurements_.insert(
        std::pair<int32_t, Measurement>(timestep, Measurement{}));
    iter = insertedIter.first;
    iter->second.start = MPI_Wtime();
    iter->second.set = false;
  }
}

void TimingMeasurement::stop(int32_t timestep) {
  double stopTime = MPI_Wtime();
  if (measurements_.find(timestep) != measurements_.end()) {
    Measurement &measurement = measurements_[timestep];
    if (!measurement.set) {
      measurement.duration = stopTime - measurement.start;
      measurement.set = true;
      TimingMeasurement::writeCSV();
    }
  }
}

void TimingMeasurement::writeCSV(const char *filename) {
  const char *fname = getenv("TIMING_FILENAME");
  int ownRankNo = DihuContext::ownRankNoCommWorld();

  // determine file name
  std::stringstream ss;
  if (fname) {
    ss << fname;
  } else {
    ss << filename;
  }
  ss << "." << std::setw(7) << std::setfill('0') << ownRankNo << ".csv";

  std::stringstream header;
  std::stringstream data;

  header << "timestep;duration" << std::endl;
  for (const std::pair<int32_t, Measurement> m : measurements_) {
    if (m.second.set) {
      data << m.first << ";" << m.second.duration << std::endl;
    }
  }

  std::ofstream out(ss.str());
  out << header.str();
  out << data.str();
  out.close();
}
} // namespace Control
