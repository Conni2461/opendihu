#include "checkpointing/python/independent.h"

namespace Checkpointing {
namespace Python {
Independent::Independent(DihuContext context,
                         std::shared_ptr<Partition::RankSubset> rankSubset,
                         const std::string &prefix, bool binary)
    : Generic(), writer_(nullptr), prefix_(prefix) {
  PyObject *d = PyDict_New();
  PyDict_SetItemString(d, "binary", binary ? Py_True : Py_False);

  writer_ = std::make_unique<OutputWriter::PythonFile>(context, PythonConfig(d),
                                                       rankSubset);
  if (prefix_ == "") {
    prefix_ = ".";
  }

  // writer_->setCombineFiles(false);
  // writer_->setWriteMeta(false);
  // writer_->setUseCheckpointData(true);
}
} // namespace Python
} // namespace Checkpointing
