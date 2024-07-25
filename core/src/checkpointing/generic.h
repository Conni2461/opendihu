#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"

namespace Checkpointing {
class Generic {
public:
  Generic() = default;
  virtual ~Generic() = default;

  bool needCheckpoint();
  bool shouldExit();

protected:
  virtual void initWriter(DihuContext context) const {}

  const int32_t maxAttempt = 3;
};
} // namespace Checkpointing
