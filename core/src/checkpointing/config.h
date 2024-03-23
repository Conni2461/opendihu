#pragma once

#include <Python.h> // has to be the first included header

#include "control/dihu_context.h"

namespace Checkpointing {
class Config {
  public:
    Config(DihuContext context, PythonConfig specificSettings);
};
}
