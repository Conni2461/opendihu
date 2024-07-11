#include "input_reader/json/file.h"

namespace InputReader {
namespace Json {
template <typename T> T File::readAttr(const char *name) const {
  return content_["__attributes"][name].template get<T>();
}
} // namespace Json
} // namespace InputReader
