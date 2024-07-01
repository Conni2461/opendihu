#include "input_reader/hdf5/file.h"

namespace InputReader {
namespace HDF5 {
File::File(const char *file) : Generic() {
  herr_t err;
  fileID_ = H5Fopen(file, H5F_ACC_RDONLY, H5P_DEFAULT);
  assert(fileID_ >= 0);

  err = H5Aiterate(fileID_, H5_INDEX_NAME, H5_ITER_INC, nullptr, attr_iter,
                   (void *)&attributes_);
  assert(err >= 0);
  err = H5Ovisit(fileID_, H5_INDEX_NAME, H5_ITER_NATIVE, obj_iter,
                 (void *)&datasets_, H5O_INFO_BASIC);
  assert(err >= 0);

  for (const auto &e : attributes_) {
    LOG(DEBUG) << "InputReader - Attribute: " << e.name << " - " << e.size;
  }
  for (const auto &e : datasets_) {
    LOG(DEBUG) << "InputReader - Dataset: " << e.name << " - " << e.type;
  }
}

File::~File() {
  herr_t err = H5Fclose(fileID_);
  assert(err >= 0);
  (void)err;
}

bool File::hasAttribute(const char *name) const {
  for (const auto &e : attributes_) {
    if (e.name == name) {
      return true;
    }
  }
  return false;
}

bool File::hasDataset(const char *name) const {
  for (const auto &e : datasets_) {
    std::string sp = e.name.substr(e.name.find("/") + 1);
    if (sp == name) {
      return true;
    }
  }
  return false;
}

int32_t File::readInt32Attribute(const char *name) const {
  hid_t attr = H5Aopen_name(fileID_, name);
  assert(attr >= 0);

  int32_t out;
  herr_t err = H5Aread(attr, H5T_NATIVE_INT, &out);
  assert(err >= 0);
  err = H5Aclose(attr);
  assert(err >= 0);
  return out;
}

double File::readDoubleAttribute(const char *name) const {
  hid_t attr = H5Aopen_name(fileID_, name);
  assert(attr >= 0);

  double out;
  herr_t err = H5Aread(attr, H5T_NATIVE_DOUBLE, &out);
  assert(err >= 0);
  err = H5Aclose(attr);
  assert(err >= 0);
  return out;
}

std::string File::readStringAttribute(const char *name) const {
  hid_t attr = H5Aopen_name(fileID_, name);
  assert(attr >= 0);

  std::string out;
  hid_t atype = H5Tcopy(H5T_C_S1);
  for (const auto &e : attributes_) {
    if (e.name == name) {
      H5Tset_size(atype, e.size + 1);
      out.resize(e.size + 1);
      break;
    }
  }

  herr_t err = H5Aread(attr, atype, (void *)out.c_str());
  assert(err >= 0);
  err = H5Tclose(atype);
  assert(err >= 0);
  err = H5Aclose(attr);
  assert(err >= 0);
  return out;
}

bool File::readIntVector(const char *name, std::vector<int32_t> &out) const {
  return false;
}

bool File::readDoubleVector(const char *name, std::vector<double> &out) const {
  return false;
}

const std::string *File::getFullDatasetName(const char *name) const {
  // same as output writer, sanatize name so it doesn't contain any `/`
  std::string dsname = name;
  std::replace(dsname.begin(), dsname.end(), '/', '|');

  for (const auto &e : datasets_) {
    std::string sp = e.name.substr(e.name.find("/") + 1);
    if (sp == dsname) {
      // Return a pointer into the attributes set, this does not need to be
      // deleted
      return &e.name;
    }
  }
  return nullptr;
}
} // namespace HDF5
} // namespace InputReader
