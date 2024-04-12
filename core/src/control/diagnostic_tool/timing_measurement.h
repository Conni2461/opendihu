#pragma once

#include <Python.h> // has to be the first included header
#include <map>

#include "control/dihu_context.h"
#include "interfaces/runnable.h"

namespace Control {

/** A class used for timing and error performance measurements. Timing is done
 * using MPI_Wtime.
 */
class TimingMeasurement {
public:
  //! start timing measurement for a given keyword
  static void start(int32_t timestep);

  //! stop timing measurement for a given keyword, the counter of number of time
  //! spans is increased by numberAccumulated
  static void stop(int32_t timestep);

  static void writeCSV(const char *filename = "timings");

private:
  struct Measurement {
    double start;
    double duration;
    bool set;
  };

  static std::map<int32_t, Measurement> measurements_;
};
} // namespace Control
