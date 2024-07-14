#include "control/diagnostic_tool/timing_measurement.h"

#include <stdlib.h>

namespace Control {
std::map<int32_t, std::map<std::string, TimingMeasurement::Measurement>>
    TimingMeasurement::measurements_;

void TimingMeasurement::start(int32_t timestep, const char *name) {
  auto innerMap = measurements_.find(timestep);

  if (innerMap == measurements_.end()) {
    auto insertedIter = measurements_.insert(
        std::pair<int32_t, std::map<std::string, Measurement>>(timestep, {}));
    innerMap = insertedIter.first;
  }
  if (innerMap->second.find(name) == innerMap->second.end()) {
    innerMap->second.insert(std::pair<std::string, Measurement>(
        name, Measurement({MPI_Wtime(), -1, false})));
  }
}

void TimingMeasurement::stop(int32_t timestep, const char *name) {
  double stopTime = MPI_Wtime();
  if (measurements_.find(timestep) == measurements_.end()) {
    return;
  }
  auto &timeStepMeasurements = measurements_[timestep];
  if (timeStepMeasurements.find(name) == timeStepMeasurements.end()) {
    return;
  }
  auto &measurement = timeStepMeasurements[name];
  if (!measurement.set) {
    measurement.duration = stopTime - measurement.start;
    LOG(INFO) << "[]: " << name
              << " setting duration: " << measurement.duration;
    measurement.set = true;
  }

  if (strcmp(name, "all") == 0) {
    TimingMeasurement::writeCSV();
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

  bool headerSet = false;
  for (const auto &tms : measurements_) {
    if (!headerSet) {
      header << "timestep";
      for (const auto &m : tms.second) {
        header << ";" << m.first;
      }
      header << std::endl;
      headerSet = true;
    }

    data << tms.first;
    for (const auto &m : tms.second) {
      data << ";";
      if (m.second.set) {
        data << m.second.duration;
      }
    }
    data << std::endl;
  }

  std::ofstream out(ss.str());
  out << header.str();
  out << data.str();
  out.close();
}
} // namespace Control
