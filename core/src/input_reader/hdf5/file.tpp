#include "input_reader/hdf5/file.h"

namespace InputReader {
namespace HDF5 {
template <typename T, std::enable_if_t<std::is_same<T, int32_t>::value, bool>>
T File::readAttr(const char *name) const {
  int32_t out;
  herr_t err = this->readAttribute(name, H5T_NATIVE_INT, (void *)&out);
  assert(err >= 0);
  (void)err;
  return out;
}

template <typename T, std::enable_if_t<std::is_same<T, double>::value, bool>>
T File::readAttr(const char *name) const {
  double out;
  herr_t err = this->readAttribute(name, H5T_NATIVE_DOUBLE, (void *)&out);
  assert(err >= 0);
  (void)err;
  return out;
}

template <typename T,
          std::enable_if_t<std::is_same<T, std::string>::value, bool>>
T File::readAttr(const char *name) const {
  std::string out;
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
  herr_t err = this->readAttribute(name, type, (void *)out.c_str());
  assert(err >= 0);
  (void)err;
  return out;
}
} // namespace HDF5
} // namespace InputReader
