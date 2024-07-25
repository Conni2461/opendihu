#include "input_reader/hdf5/file.h"

namespace InputReader {
namespace HDF5 {
template <typename T, std::enable_if_t<std::is_same<T, int32_t>::value, bool>>
bool File::readAttr(const char *name, T &out) const {
  return this->readAttribute(name, H5T_NATIVE_INT, (void *)&out) >= 0;
}

template <typename T, std::enable_if_t<std::is_same<T, double>::value, bool>>
bool File::readAttr(const char *name, T &out) const {
  return this->readAttribute(name, H5T_NATIVE_DOUBLE, (void *)&out) >= 0;
}

template <typename T,
          std::enable_if_t<std::is_same<T, std::string>::value, bool>>
bool File::readAttr(const char *name, T &out) const {
  hid_t type = H5Tcopy(H5T_C_S1);
  bool found = false;
  for (const auto &e : attributes_) {
    if (e.name == name) {
      H5Tset_size(type, e.size + 1);
      out.resize(e.size + 1);
      found = true;
      break;
    }
  }
  assert(found);
  return this->readAttribute(name, type, (void *)out.c_str()) >= 0;
}
} // namespace HDF5
} // namespace InputReader
