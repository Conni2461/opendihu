#include "input_reader/generic.h"

namespace InputReader {
template <int D>
bool Generic::readDoubleVecD(const char *name, int maxSize,
                             std::vector<VecD<D>> &out,
                             const std::string &groupName) const {
  if (maxSize >= 0) {
    if (maxSize % D != 0) {
      return false;
    }
  }

  std::vector<double> values;
  bool ret = this->readDoubleVector(name, values, groupName);
  if (!ret) {
    return ret;
  }

  if (maxSize < 0) {
    maxSize = values.size();
    if (maxSize % D != 0) {
      return false;
    }
  }

  out.reserve(maxSize / D);
  for (size_t i = 0; i < maxSize; i += D) {
    VecD<D> mat;
    for (size_t j = 0; j < D; j++) {
      mat[j] = values[i + j];
    }
    out.push_back(mat);
  }

  return true;
}
} // namespace InputReader
