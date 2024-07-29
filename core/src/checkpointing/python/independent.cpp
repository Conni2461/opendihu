#include "checkpointing/python/independent.h"

namespace Checkpointing {
namespace Python {
Independent::Independent(DihuContext context,
                         std::shared_ptr<Partition::RankSubset> rankSubset,
                         const std::string &prefix, bool binary)
    : Generic(prefix), writer_(nullptr) {
  PyObject *d = PyDict_New();
  PyDict_SetItemString(d, "binary", binary ? Py_True : Py_False);

  writer_ = std::make_unique<OutputWriter::PythonFile>(context, PythonConfig(d),
                                                       rankSubset);
  // writer_->setCombineFiles(false);
  // writer_->setWriteMeta(false);
  // writer_->setUseCheckpointData(true);
}
} // namespace Python
} // namespace Checkpointing
