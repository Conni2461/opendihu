#include "output_writer/hdf5/hdf5.h"

#include "output_writer/loop_count_n_field_variables_of_mesh.h"
#include "output_writer/hdf5/loop_output.h"

namespace OutputWriter {
template <typename DataType>
void HDF5::write(DataType &data, const char *filename, int timeStepNo,
                 double currentTime, int callCountIncrement) {
  // check if output should be written in this timestep and prepare filename
  if (!Generic::prepareWrite(data, timeStepNo, currentTime,
                             callCountIncrement)) {
    return;
  }
  this->innerWrite(data.getFieldVariablesForOutputWriter(), filename,
                   timeStepNo, currentTime, callCountIncrement);
}

template <typename FieldVariablesForOutputWriterType>
void HDF5::innerWrite(const FieldVariablesForOutputWriterType &variable,
                      const char *filename, int timeStepNo, double currentTime,
                      int callCountIncrement) {
  Control::PerformanceMeasurement::start("durationHDF5Output");

  std::set<std::string> combined1DMeshes;
  std::set<std::string> combined2DMeshes;
  std::set<std::string> combined3DMeshes;

  if (combineFiles_) {
    hid_t fileID;
    if (!filename) {
      // determine filename, broadcast from rank 0
      std::stringstream filename;
      filename << this->filenameBaseWithNo_ << "_c.h5";
      int filenameLength = filename.str().length();

      // broadcast length of filename
      MPIUtility::handleReturnValue(
          MPI_Bcast(&filenameLength, 1, MPI_INT, 0,
                    this->rankSubset_->mpiCommunicator()),
          "MPI_Bcast");

      std::vector<char> receiveBuffer(filenameLength + 1, char(0));
      strcpy(receiveBuffer.data(), filename.str().c_str());
      MPIUtility::handleReturnValue(
          MPI_Bcast(receiveBuffer.data(), filenameLength, MPI_CHAR, 0,
                    this->rankSubset_->mpiCommunicator()),
          "MPI_Bcast");
      LOG(DEBUG) << "open HDF5 file using MPI IO \"" << receiveBuffer.data()
                 << "\".";
      fileID = openHDF5File(receiveBuffer.data(), true);
    } else {
      fileID = openHDF5File(filename, true);
    }

    herr_t err;
    if (writeMeta_) {
      err = HDF5Utils::writeAttr<const std::string &>(
          fileID, "version", DihuContext::versionText());
      assert(err >= 0);
      err = HDF5Utils::writeAttr<const std::string &>(fileID, "meta",
                                                      DihuContext::metaText());
      assert(err >= 0);
    }
    err = HDF5Utils::writeAttr(fileID, "currentTime", this->currentTime_);
    assert(err >= 0);
    err = HDF5Utils::writeAttr(fileID, "timeStepNo", this->timeStepNo_);
    assert(err >= 0);

    Control::PerformanceMeasurement::start("durationHDF51D");

    LOG(DEBUG) << "FieldVariablesForOutputWriterType: "
               << StringUtility::demangle(
                      typeid(FieldVariablesForOutputWriterType).name());

    // create a PolyData file that combines all 1D meshes into one file
    writePolyDataFile<FieldVariablesForOutputWriterType>(fileID, variable,
                                                         combined1DMeshes);

    Control::PerformanceMeasurement::stop("durationHDF51D");
    Control::PerformanceMeasurement::start("durationHDF53D");

    // create an UnstructuredMesh file that combines all 3D meshes into one file
    writeCombinedUnstructuredGridFile<FieldVariablesForOutputWriterType>(
        fileID, variable, combined3DMeshes, true);

    Control::PerformanceMeasurement::stop("durationHDF53D");
    Control::PerformanceMeasurement::start("durationHDF52D");

    // create an UnstructuredMesh file that combines all 2D meshes into one file
    writeCombinedUnstructuredGridFile<FieldVariablesForOutputWriterType>(
        fileID, variable, combined2DMeshes, false);

    Control::PerformanceMeasurement::stop("durationHDF52D");

    err = H5Fclose(fileID);
    assert(err >= 0);
  }

  // output normal files, parallel or if combineFiles_, only the 2D and 3D
  // meshes, combined

  // collect all available meshes
  std::set<std::string> meshNames;
  LoopOverTuple::loopCollectMeshNames<FieldVariablesForOutputWriterType>(
      variable, meshNames);

  // remove 1D meshes that were already output by writePolyDataFile
  std::set<std::string> meshesWithout1D;
  std::set_difference(meshNames.begin(), meshNames.end(),
                      combined1DMeshes.begin(), combined1DMeshes.end(),
                      std::inserter(meshesWithout1D, meshesWithout1D.end()));

  // remove 3D meshes that were already output by
  // writeCombinedUnstructuredGridFile
  std::set<std::string> meshesWithout1D3D;
  std::set_difference(
      meshesWithout1D.begin(), meshesWithout1D.end(), combined3DMeshes.begin(),
      combined3DMeshes.end(),
      std::inserter(meshesWithout1D3D, meshesWithout1D3D.end()));

  // remove 2D meshes that were already output by
  // writeCombinedUnstructuredGridFile
  std::set<std::string> meshesToOutput;
  std::set_difference(meshesWithout1D3D.begin(), meshesWithout1D3D.end(),
                      combined2DMeshes.begin(), combined2DMeshes.end(),
                      std::inserter(meshesToOutput, meshesToOutput.end()));

  if (meshesToOutput.size() > 0) {
    // extract filename base
    std::stringstream s;
    if (!filename) {
      s << filename << "p";
    } else {
      s << this->filename_ << "_p.h5";
    }

    hid_t fileID = openHDF5File(s.str().c_str(), true);
    herr_t err;
    if (writeMeta_) {
      err = HDF5Utils::writeAttr<const std::string &>(
          fileID, "version", DihuContext::versionText());
      assert(err >= 0);
      err = HDF5Utils::writeAttr<const std::string &>(fileID, "meta",
                                                      DihuContext::metaText());
      assert(err >= 0);
    }
    err = HDF5Utils::writeAttr(fileID, "currentTime", this->currentTime_);
    assert(err >= 0);
    err = HDF5Utils::writeAttr(fileID, "timeStepNo", this->timeStepNo_);
    assert(err >= 0);
    for (const std::string &meshName : meshesToOutput) {
      hid_t groupID = H5Gcreate(fileID, meshName.c_str(), H5P_DEFAULT,
                                H5P_DEFAULT, H5P_DEFAULT);
      assert(groupID >= 0);

      // loop over all field variables and output those that are associated with
      // the mesh given by meshName
      HDF5LoopOverTuple::loopOutput(groupID, variable, variable, meshName,
                                    specificSettings_, currentTime);

      err = H5Gclose(groupID);
      assert(err >= 0);
    }
  }

  Control::PerformanceMeasurement::stop("durationHDF5Output");
}

namespace HDF5Utils {
template <typename T> herr_t writeAttr(hid_t fileID, const char *key, T value) {
  assert(false);
  return 0;
}

template <>
herr_t writeAttr<int32_t>(hid_t fileID, const char *key, int32_t value) {
  std::array<hsize_t, 1> dims = {1};
  std::array<int32_t, 1> data = {value};
  hid_t dspace = H5Screate_simple(1, dims.data(), nullptr);
  if (dspace < 0) {
    return dspace;
  }
  hid_t attr =
      H5Acreate(fileID, key, H5T_STD_I32BE, dspace, H5P_DEFAULT, H5P_DEFAULT);
  if (attr < 0) {
    // ignore error here, everything is lost anyway at this point in time
    H5Sclose(dspace);
    return attr;
  }
  herr_t err = H5Awrite(attr, H5T_NATIVE_INT, data.data());
  if (err < 0) {
    return err;
  }
  err = H5Aclose(attr);
  if (err < 0) {
    return err;
  }
  err = H5Sclose(dspace);
  if (err < 0) {
    return err;
  }

  return 0;
}
template <>
herr_t writeAttr<double>(hid_t fileID, const char *key, double value) {
  std::array<hsize_t, 1> dims = {1};
  std::array<double, 1> data = {value};
  hid_t dspace = H5Screate_simple(1, dims.data(), nullptr);
  if (dspace < 0) {
    return dspace;
  }
  hid_t attr =
      H5Acreate(fileID, key, H5T_IEEE_F64BE, dspace, H5P_DEFAULT, H5P_DEFAULT);
  if (attr < 0) {
    // ignore error here, everything is lost anyway at this point in time
    H5Sclose(dspace);
    return attr;
  }
  herr_t err = H5Awrite(attr, H5T_NATIVE_DOUBLE, data.data());
  if (err < 0) {
    return err;
  }
  err = H5Aclose(attr);
  if (err < 0) {
    return err;
  }
  err = H5Sclose(dspace);
  if (err < 0) {
    return err;
  }

  return 0;
}
template <>
herr_t writeAttr<const std::string &>(hid_t fileID, const char *key,
                                      const std::string &value) {
  hid_t filetype = H5Tcopy(H5T_FORTRAN_S1);
  herr_t err = H5Tset_size(filetype, value.length());
  if (err < 0) {
    return err;
  }

  hid_t memtype = H5Tcopy(H5T_C_S1); // Datatype ID
  err = H5Tset_size(memtype, value.length() + 1);
  if (err < 0) {
    return err;
  }

  std::array<hsize_t, 1> dims = {1};
  hid_t dspace = H5Screate_simple(1, dims.data(), nullptr);
  if (dspace < 0) {
    return dspace;
  }
  hid_t attr =
      H5Acreate(fileID, key, filetype, dspace, H5P_DEFAULT, H5P_DEFAULT);
  if (attr < 0) {
    // ignore error here, everything is lost anyway at this point in time
    H5Sclose(dspace);
    return attr;
  }
  err = H5Awrite(attr, memtype, value.c_str());
  if (err < 0) {
    H5Sclose(dspace);
    H5Aclose(attr);
    return err;
  }
  err = H5Aclose(attr);
  if (err < 0) {
    return err;
  }
  err = H5Sclose(dspace);
  if (err < 0) {
    return err;
  }

  return 0;
}

template <typename T>
herr_t writeSimpleVec(hid_t fileID, const std::vector<T> &data,
                      const char *dsname) {
  assert(false);
  return 0;
}

template <>
herr_t writeSimpleVec<int32_t>(hid_t fileID, const std::vector<int32_t> &data,
                               const char *dsname) {
  herr_t err;
  std::array<hsize_t, 1> dims = {data.size()};
  hid_t dspace = H5Screate_simple(1, dims.data(), nullptr);
  if (dspace < 0) {
    return dspace;
  }
  hid_t dset = H5Dcreate(fileID, dsname, H5T_STD_I32BE, dspace, H5P_DEFAULT,
                         H5P_DEFAULT, H5P_DEFAULT);
  if (dset < 0) {
    // ignore error here, everything is lost anyway at this point in time
    H5Sclose(dspace);
    return dset;
  }
  err = H5Dwrite(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                 data.data());
  if (err < 0) {
    H5Dclose(dset);
    H5Sclose(dspace);
    return err;
  }

  err = H5Dclose(dset);
  if (err < 0) {
    return err;
  }

  err = H5Sclose(dspace);
  if (err < 0) {
    return err;
  }

  return 0;
}

template <>
herr_t writeSimpleVec<double>(hid_t fileID, const std::vector<double> &data,
                              const char *dsname) {
  herr_t err;
  std::array<hsize_t, 1> dims = {data.size()};
  hid_t dspace = H5Screate_simple(1, dims.data(), nullptr);
  if (dspace < 0) {
    return dspace;
  }

  hid_t dset = H5Dcreate(fileID, dsname, H5T_IEEE_F64BE, dspace, H5P_DEFAULT,
                         H5P_DEFAULT, H5P_DEFAULT);
  if (dset < 0) {
    // ignore error here, everything is lost anyway at this point in time
    H5Sclose(dspace);
    return dset;
  }
  err = H5Dwrite(dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                 data.data());
  if (err < 0) {
    H5Dclose(dset);
    H5Sclose(dspace);
    return err;
  }
  err = H5Dclose(dset);
  if (err < 0) {
    return err;
  }

  err = H5Sclose(dspace);
  if (err < 0) {
    return err;
  }

  return 0;
}

template <typename FieldVariableType>
herr_t writeFieldVariable(hid_t fileID, FieldVariableType &fieldVariable) {
  LOG(DEBUG) << "HDF5 write field variable " << fieldVariable.name();
  VLOG(1) << fieldVariable;

  int nComponentsH5 = fieldVariable.nComponents();

  const int nComponents = FieldVariableType::nComponents();
  std::string stringData;

  std::vector<double> values;
  std::array<std::vector<double>, nComponents> componentValues;

  // ensure that ghost values are in place
  Partition::values_representation_t old_representation =
      fieldVariable.currentRepresentation();
  fieldVariable.zeroGhostBuffer();
  fieldVariable.setRepresentationGlobal();
  fieldVariable.startGhostManipulation();

  // get all local values including ghosts for the components
  for (int componentNo = 0; componentNo < nComponents; componentNo++) {
    std::vector<double> retrievedLocalValues;
    fieldVariable.getValues(componentNo,
                            fieldVariable.functionSpace()
                                ->meshPartition()
                                ->dofNosLocalNaturalOrdering(),
                            retrievedLocalValues);

    const int nDofsPerNode = FieldVariableType::FunctionSpace::nDofsPerNode();
    const node_no_t nNodesLocal =
        fieldVariable.functionSpace()->meshPartition()->nNodesLocalWithGhosts();

    // for Hermite only extract the non-derivative values
    componentValues[componentNo].resize(nNodesLocal);

    int index = 0;
    for (int i = 0; i < nNodesLocal; i++) {
      componentValues[componentNo][i] = retrievedLocalValues[index];
      index += nDofsPerNode;
    }
  }

  // reset variable to old representation, so external code is not suprised
  // eg. time stepping code usally uses representationContiguous and will be
  // suprised if this changed we did not write to the values
  fieldVariable.setRepresentation(old_representation,
                                  values_modified_t::values_unchanged);

  // copy values in consecutive order (x y z x y z) to output
  values.reserve(componentValues[0].size() * nComponentsH5);
  for (int i = 0; i < componentValues[0].size(); i++) {
    for (int componentNo = 0; componentNo < nComponentsH5; componentNo++) {
      if (nComponents == 2 && nComponentsH5 == 3 && componentNo == 2) {
        values.push_back(0.0);
      } else {
        values.push_back(componentValues[componentNo][i]);
      }
    }
  }

  herr_t err;
  std::array<hsize_t, 2> dims = {nComponents, componentValues[0].size()};
  hid_t dspace = H5Screate_simple(2, dims.data(), nullptr);
  if (dspace < 0) {
    return dspace;
  }

  hid_t dset = H5Dcreate(fileID, fieldVariable.name().c_str(), H5T_STD_I32BE,
                         dspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (dset < 0) {
    // ignore error here, everything is lost anyway at this point in time
    H5Sclose(dspace);
    return dset;
  }
  err = H5Dwrite(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                 values.data());
  if (err < 0) {
    H5Dclose(dset);
    H5Sclose(dspace);
    return err;
  }
  err = H5Dclose(dset);
  if (err < 0) {
    return err;
  }

  err = H5Sclose(dspace);
  if (err < 0) {
    return err;
  }

  return 0;
}

template <typename FieldVariableType>
herr_t writePartitionFieldVariable(hid_t fileID,
                                   FieldVariableType &geometryField) {
  const node_no_t nNodesLocal =
      geometryField.functionSpace()->meshPartition()->nNodesLocalWithGhosts();

  std::vector<int32_t> values(nNodesLocal,
                              (int32_t)DihuContext::ownRankNoCommWorld());
  return writeSimpleVec(fileID, values, "partitioning");
}
} // namespace HDF5Utils
} // namespace OutputWriter
