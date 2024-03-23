#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"

namespace Checkpointing {
class Generic {
public:
  Generic(DihuContext context, PythonConfig specificSettings);
private:

};
} // namespace Checkpointing
