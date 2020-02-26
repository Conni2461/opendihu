#include "output_writer/paraview/paraview.h"

#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>  // remove

#include "easylogging++.h"
#include "base64.h"

#include "output_writer/paraview/loop_output.h"
#include "output_writer/paraview/loop_collect_mesh_properties.h"
#include "output_writer/paraview/loop_get_nodal_values.h"
#include "output_writer/paraview/loop_get_geometry_field_nodal_values.h"
#include "output_writer/paraview/poly_data_properties_for_mesh.h"
#include "control/diagnostic_tool/performance_measurement.h"

namespace OutputWriter
{

template<typename FieldVariablesForOutputWriterType>
void Paraview::writePolyDataFile(const FieldVariablesForOutputWriterType &fieldVariables, std::set<std::string> &meshNames)
{
  // output a *.vtp file which contains 1D meshes, if there are any

  bool meshPropertiesInitialized = !meshPropertiesPolyDataFile_.empty();

  if (!meshPropertiesInitialized)
  {
    Control::PerformanceMeasurement::start("durationParaview1DInit");

    // collect the size data that is needed to compute offsets for parallel file output
    ParaviewLoopOverTuple::loopCollectMeshProperties<FieldVariablesForOutputWriterType>(fieldVariables, meshPropertiesPolyDataFile_);

    Control::PerformanceMeasurement::stop("durationParaview1DInit");
  }

  VLOG(1) << "writePolyDataFile on rankSubset_: " << *this->rankSubset_;
  assert(this->rankSubset_);

  VLOG(1) << "meshPropertiesPolyDataFile_: " << meshPropertiesPolyDataFile_;
  /*
  struct PolyDataPropertiesForMesh
  {
    int dimensionality;    ///< D=1: object is a VTK "Line", D=2, D=3: object is a VTK "Poly"
    global_no_t nPointsLocal;   ///< the number of points needed for representing the mesh, local value of rank
    global_no_t nCellsLocal;    ///< the number of VTK "cells", i.e. "Lines" or "Polys", which is the opendihu number of "elements", local value of rank
    global_no_t nPointsGlobal;   ///< the number of points needed for representing the mesh, global value of all rank
    global_no_t nCellsGlobal;    ///< the number of VTK "cells", i.e. "Lines" or "Polys", which is the opendihu number of "elements", global value of all ranks

    std::vector<std::pair<std::string,int>> pointDataArrays;   ///< <name,nComponents> of PointData DataArray elements
  };
*/

  /* one VTKPiece is the XML element that will be output as <Piece></Piece>. It is created from one or multiple opendihu meshes
   */
  /*
  struct VTKPiece
  {
    std::set<std::string> meshNamesCombinedMeshes;   ///< the meshNames of the combined meshes, or only one meshName if it is not a merged mesh
    PolyDataPropertiesForMesh properties;   ///< the properties of the merged mesh

    std::string firstScalarName;   ///< name of the first scalar field variable of the mesh
    std::string firstVectorName;   ///< name of the first non-scalar field variable of the mesh

    //! constructor, initialize nPoints and nCells to 0
    VTKPiece()
    {
      properties.nPointsLocal = 0;
      properties.nCellsLocal = 0;
      properties.nPointsGlobal = 0;
      properties.nCellsGlobal = 0;
      properties.dimensionality = 0;
    }

    //! assign the correct values to firstScalarName and firstVectorName, only if properties has been set
    void setVTKValues()
    {
      // set values for firstScalarName and firstVectorName from the values in pointDataArrays
      for (auto pointDataArray : properties.pointDataArrays)
      {
        if (firstScalarName == "" && pointDataArray.second == 1)
          firstScalarName = pointDataArray.first;
        if (firstVectorName == "" && pointDataArray.second != 1)
          firstVectorName = pointDataArray.first;
      }
    }
  } vtkPiece;
  */

  if (!meshPropertiesInitialized)
  {
    Control::PerformanceMeasurement::start("durationParaview1DInit");

    // parse the collected properties of the meshes that will be output to the file
    for (std::map<std::string,PolyDataPropertiesForMesh>::iterator iter = meshPropertiesPolyDataFile_.begin(); iter != meshPropertiesPolyDataFile_.end(); iter++)
    {
      std::string meshName = iter->first;

      // do not combine meshes other than 1D meshes
      if (iter->second.dimensionality != 1)
        continue;

      // check if this mesh should be combined with other meshes
      bool combineMesh = true;

      // check if mesh can be merged into previous meshes
      if (!vtkPiece_.properties.pointDataArrays.empty())   // if properties are already assigned by an earlier mesh
      {
        if (vtkPiece_.properties.pointDataArrays.size() != iter->second.pointDataArrays.size())
        {
          LOG(DEBUG) << "Mesh " << meshName << " cannot be combined with " << vtkPiece_.meshNamesCombinedMeshes << ". Number of field variables mismatches for "
            << meshName << " (is " << iter->second.pointDataArrays.size() << " instead of " << vtkPiece_.properties.pointDataArrays.size() << ")";
          combineMesh = false;
        }
        else
        {
          for (int j = 0; j < iter->second.pointDataArrays.size(); j++)
          {
            if (vtkPiece_.properties.pointDataArrays[j].first != iter->second.pointDataArrays[j].first)  // if the name of the jth field variable is different
            {
              LOG(DEBUG) << "Mesh " << meshName << " cannot be combined with " << vtkPiece_.meshNamesCombinedMeshes << ". Field variable names mismatch for "
                << meshName << " (there is \"" << vtkPiece_.properties.pointDataArrays[j].first << "\" instead of \"" << iter->second.pointDataArrays[j].first << "\")";
              combineMesh = false;
            }
          }
        }

        if (combineMesh)
        {
          VLOG(1) << "Combine mesh " << meshName << " with " << vtkPiece_.meshNamesCombinedMeshes
            << ", add " << iter->second.nPointsLocal << " points, " << iter->second.nCellsLocal << " elements to "
            << vtkPiece_.properties.nPointsLocal << " points, " << vtkPiece_.properties.nCellsLocal << " elements";

          vtkPiece_.properties.nPointsLocal += iter->second.nPointsLocal;
          vtkPiece_.properties.nCellsLocal += iter->second.nCellsLocal;

          vtkPiece_.properties.nPointsGlobal += iter->second.nPointsGlobal;
          vtkPiece_.properties.nCellsGlobal += iter->second.nCellsGlobal;
          vtkPiece_.setVTKValues();
        }
      }
      else
      {
        VLOG(1) << "this is the first 1D mesh";

        // properties are not yet assigned
        vtkPiece_.properties = iter->second; // store properties
        vtkPiece_.setVTKValues();
      }

      VLOG(1) << "combineMesh: " << combineMesh;
      if (combineMesh)
      {
        vtkPiece_.meshNamesCombinedMeshes.insert(meshName);
      }
    }

    LOG(DEBUG) << "vtkPiece_: meshNamesCombinedMeshes: " << vtkPiece_.meshNamesCombinedMeshes << ", properties: " << vtkPiece_.properties
      << ", firstScalarName: " << vtkPiece_.firstScalarName << ", firstVectorName: " << vtkPiece_.firstVectorName;

    Control::PerformanceMeasurement::stop("durationParaview1DInit");
  }

  meshNames = vtkPiece_.meshNamesCombinedMeshes;

  // if there are no 1D meshes, return
  if (meshNames.empty())
    return;

  if (!meshPropertiesInitialized)
  {
    // add field variable "partitioning" with 1 component
    vtkPiece_.properties.pointDataArrays.push_back(std::pair<std::string,int>("partitioning", 1));
  }

  // determine filename, broadcast from rank 0
  std::stringstream filename;
  filename << this->filenameBaseWithNo_ << ".vtp";
  int filenameLength = filename.str().length();

  // broadcast length of filename
  MPIUtility::handleReturnValue(MPI_Bcast(&filenameLength, 1, MPI_INT, 0, this->rankSubset_->mpiCommunicator()));

  std::vector<char> receiveBuffer(filenameLength+1, char(0));
  strcpy(receiveBuffer.data(), filename.str().c_str());
  MPIUtility::handleReturnValue(MPI_Bcast(receiveBuffer.data(), filenameLength, MPI_CHAR, 0, this->rankSubset_->mpiCommunicator()));

  std::string filenameStr(receiveBuffer.begin(), receiveBuffer.end());

  // remove file if it exists, synchronization afterwards by MPI calls, that is why the remove call is already here
  assert(this->rankSubset_);
  int ownRankNo = this->rankSubset_->ownRankNo();
  if (ownRankNo == 0)
  {
    // open file to ensure that directory exists and file is writable
    std::ofstream file;
    Generic::openFile(file, filenameStr);

    // close and delete file
    file.close();
    std::remove(filenameStr.c_str());
  }

  if (!meshPropertiesInitialized)
  {
    // exchange information about offset in terms of nCells and nPoints
    nCellsPreviousRanks1D_ = 0;
    nPointsPreviousRanks1D_ = 0;
    nPointsGlobal1D_ = 0;
    nLinesGlobal1D_ = 0;

    Control::PerformanceMeasurement::start("durationParaview1DInit");
    Control::PerformanceMeasurement::start("durationParaview1DReduction");
    MPIUtility::handleReturnValue(MPI_Exscan(&vtkPiece_.properties.nCellsLocal, &nCellsPreviousRanks1D_, 1, MPI_INT, MPI_SUM, this->rankSubset_->mpiCommunicator()), "MPI_Exscan");
    MPIUtility::handleReturnValue(MPI_Exscan(&vtkPiece_.properties.nPointsLocal, &nPointsPreviousRanks1D_, 1, MPI_INT, MPI_SUM, this->rankSubset_->mpiCommunicator()), "MPI_Exscan");
    MPIUtility::handleReturnValue(MPI_Reduce(&vtkPiece_.properties.nPointsLocal, &nPointsGlobal1D_, 1, MPI_INT, MPI_SUM, 0, this->rankSubset_->mpiCommunicator()), "MPI_Reduce");
    MPIUtility::handleReturnValue(MPI_Reduce(&vtkPiece_.properties.nCellsLocal, &nLinesGlobal1D_, 1, MPI_INT, MPI_SUM, 0, this->rankSubset_->mpiCommunicator()), "MPI_Reduce");
    Control::PerformanceMeasurement::stop("durationParaview1DReduction");
    Control::PerformanceMeasurement::stop("durationParaview1DInit");
  }

  // get local data values
  // setup connectivity array
  std::vector<int> connectivityValues(2*vtkPiece_.properties.nCellsLocal);
  for (int i = 0; i < vtkPiece_.properties.nCellsLocal; i++)
  {
    connectivityValues[2*i + 0] = nPointsPreviousRanks1D_ + i;
    connectivityValues[2*i + 1] = nPointsPreviousRanks1D_ + i+1;
  }

  // setup offset array
  std::vector<int> offsetValues(vtkPiece_.properties.nCellsLocal);
  for (int i = 0; i < vtkPiece_.properties.nCellsLocal; i++)
  {
    offsetValues[i] = 2*nCellsPreviousRanks1D_ + 2*i + 1;
  }

  // collect all data for the field variables, organized by field variable names
  std::map<std::string, std::vector<double>> fieldVariableValues;
  ParaviewLoopOverTuple::loopGetNodalValues<FieldVariablesForOutputWriterType>(fieldVariables, vtkPiece_.meshNamesCombinedMeshes, fieldVariableValues);

  assert (!fieldVariableValues.empty());
  fieldVariableValues["partitioning"].resize(vtkPiece_.properties.nPointsLocal, (double)this->rankSubset_->ownRankNo());

  // if next assertion will fail, output why for debugging
  if (fieldVariableValues.size() != vtkPiece_.properties.pointDataArrays.size())
  {
    LOG(DEBUG) << "n field variable values: " << fieldVariableValues.size() << ", n point data arrays: "
      << vtkPiece_.properties.pointDataArrays.size();
    LOG(DEBUG) << "vtkPiece_.meshNamesCombinedMeshes: " << vtkPiece_.meshNamesCombinedMeshes;
    std::stringstream pointDataArraysNames;
    for (int i = 0; i < vtkPiece_.properties.pointDataArrays.size(); i++)
    {
      pointDataArraysNames << vtkPiece_.properties.pointDataArrays[i].first << " ";
    }
    LOG(DEBUG) << "pointDataArraysNames: " <<  pointDataArraysNames.str();
  }

  assert(fieldVariableValues.size() == vtkPiece_.properties.pointDataArrays.size());

#ifndef NDEBUG
  LOG(DEBUG) << "fieldVariableValues: ";
  for (std::map<std::string, std::vector<double>>::iterator iter = fieldVariableValues.begin(); iter != fieldVariableValues.end(); iter++)
  {
    LOG(DEBUG) << iter->first;
  }
#endif

  // check if field variable names have changed since last initialization
  for (std::vector<std::pair<std::string,int>>::iterator pointDataArrayIter = vtkPiece_.properties.pointDataArrays.begin();
       pointDataArrayIter != vtkPiece_.properties.pointDataArrays.end(); pointDataArrayIter++)
  {
    LOG(DEBUG) << "  field variable \"" << pointDataArrayIter->first << "\".";

    // if there is a field variable with a name that was not present when vtkPiece_ was created
    if (fieldVariableValues.find(pointDataArrayIter->first) == fieldVariableValues.end())
    {
      LOG(DEBUG) << "Field variable names have changed, reinitialize Paraview output writer.";

      // reset now old variables
      meshPropertiesInitialized = false;
      meshPropertiesPolyDataFile_.clear();
      vtkPiece_ = VTKPiece();

      // recursively call this method
      writePolyDataFile(fieldVariables, meshNames);

      return;
    }
  }

  // collect all data for the geometry field variable
  std::vector<double> geometryFieldValues;
  ParaviewLoopOverTuple::loopGetGeometryFieldNodalValues<FieldVariablesForOutputWriterType>(fieldVariables, vtkPiece_.meshNamesCombinedMeshes, geometryFieldValues);

  // only continue if there is data to reduce
  if (vtkPiece_.meshNamesCombinedMeshes.empty())
  {
    LOG(ERROR) << "There are no 1D meshes that could be combined, but Paraview output with combineFiles=True was specified. \n(This only works for 1D meshes.)";
  }

  LOG(DEBUG) << "Combined mesh from " << vtkPiece_.meshNamesCombinedMeshes;

  int nOutputFileParts = 4 + vtkPiece_.properties.pointDataArrays.size();

  // transform current time to string
  std::vector<double> time(1, this->currentTime_);
  std::string stringTime;
  if (binaryOutput_)
  {
    stringTime = Paraview::encodeBase64Float(time.begin(), time.end());
  }
  else
  {
    stringTime = Paraview::convertToAscii(time, fixedFormat_);
  }

  // create the basic structure of the output file
  std::vector<std::stringstream> outputFileParts(nOutputFileParts);
  int outputFilePartNo = 0;
  outputFileParts[outputFilePartNo] << "<?xml version=\"1.0\"?>" << std::endl
    << "<!-- " << DihuContext::versionText() << " " << DihuContext::metaText()
    << ", currentTime: " << this->currentTime_ << ", timeStepNo: " << this->timeStepNo_ << " -->" << std::endl
    << "<VTKFile type=\"PolyData\" version=\"1.0\" byte_order=\"LittleEndian\">" << std::endl    // intel cpus are LittleEndian
    << std::string(1, '\t') << "<PolyData>" << std::endl
    << std::string(2, '\t') << "<FieldData>" << std::endl
    << std::string(3, '\t') << "<DataArray type=\"Float32\" Name=\"Time\" NumberOfTuples=\"1\" format=\"" << (binaryOutput_? "binary" : "ascii")
    << "\" >" << std::endl
    << std::string(4, '\t') << stringTime << std::endl
    << std::string(3, '\t') << "</DataArray>" << std::endl
    << std::string(2, '\t') << "</FieldData>" << std::endl;

  outputFileParts[outputFilePartNo] << std::string(2, '\t') << "<Piece NumberOfPoints=\"" << nPointsGlobal1D_ << "\" NumberOfVerts=\"0\" "
    << "NumberOfLines=\"" << nLinesGlobal1D_ << "\" NumberOfStrips=\"0\" NumberOfPolys=\"0\">" << std::endl
    << std::string(3, '\t') << "<PointData";

  if (vtkPiece_.firstScalarName != "")
  {
    outputFileParts[outputFilePartNo] << " Scalars=\"" << vtkPiece_.firstScalarName << "\"";
  }
  if (vtkPiece_.firstVectorName != "")
  {
    outputFileParts[outputFilePartNo] << " Vectors=\"" << vtkPiece_.firstVectorName << "\"";
  }
  outputFileParts[outputFilePartNo] << ">" << std::endl;

  // loop over field variables (PointData)
  for (std::vector<std::pair<std::string,int>>::iterator pointDataArrayIter = vtkPiece_.properties.pointDataArrays.begin(); pointDataArrayIter != vtkPiece_.properties.pointDataArrays.end(); pointDataArrayIter++)
  {
    // write normal data element
    outputFileParts[outputFilePartNo] << std::string(4, '\t') << "<DataArray "
        << "Name=\"" << pointDataArrayIter->first << "\" "
        << "type=\"" << (pointDataArrayIter->first == "partitioning"? "Int32" : "Float32") << "\" "
        << "NumberOfComponents=\"" << pointDataArrayIter->second << "\" format=\"" << (binaryOutput_? "binary" : "ascii")
        << "\" >" << std::endl << std::string(5, '\t');

    // at this point the data of the field variable is missing
    outputFilePartNo++;

    outputFileParts[outputFilePartNo] << std::endl << std::string(4, '\t') << "</DataArray>" << std::endl;
  }

  outputFileParts[outputFilePartNo] << std::string(3, '\t') << "</PointData>" << std::endl
    << std::string(3, '\t') << "<CellData>" << std::endl
    << std::string(3, '\t') << "</CellData>" << std::endl
    << std::string(3, '\t') << "<Points>" << std::endl
    << std::string(4, '\t') << "<DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"" << (binaryOutput_? "binary" : "ascii")
    << "\" >" << std::endl << std::string(5, '\t');

  // at this point the data of points (geometry field) is missing
  outputFilePartNo++;

  outputFileParts[outputFilePartNo] << std::endl << std::string(4, '\t') << "</DataArray>" << std::endl
    << std::string(3, '\t') << "</Points>" << std::endl
    << std::string(3, '\t') << "<Verts></Verts>" << std::endl
    << std::string(3, '\t') << "<Lines>" << std::endl
    << std::string(4, '\t') << "<DataArray Name=\"connectivity\" type=\"Int32\" "
    << (binaryOutput_? "format=\"binary\"" : "format=\"ascii\"") << ">" << std::endl << std::string(5, '\t');

  // at this point the the structural information of the lines (connectivity) is missing
  outputFilePartNo++;

  outputFileParts[outputFilePartNo]
    << std::endl << std::string(4, '\t') << "</DataArray>" << std::endl
    << std::string(4, '\t') << "<DataArray Name=\"offsets\" type=\"Int32\" "
    << (binaryOutput_? "format=\"binary\"" : "format=\"ascii\"") << ">" << std::endl << std::string(5, '\t');

  // at this point the offset array will be written to the file
  outputFilePartNo++;

  outputFileParts[outputFilePartNo]
    << std::endl << std::string(4, '\t') << "</DataArray>" << std::endl
    << std::string(3, '\t') << "</Lines>" << std::endl
    << std::string(3, '\t') << "<Strips></Strips>" << std::endl
    << std::string(3, '\t') << "<Polys></Polys>" << std::endl
    << std::string(2, '\t') << "</Piece>" << std::endl
    << std::string(1, '\t') << "</PolyData>" << std::endl
    << "</VTKFile>" << std::endl;

  assert(outputFilePartNo+1 == nOutputFileParts);

  // loop over output file parts and collect the missing data for the own rank

  VLOG(1) << "outputFileParts:";

  for (std::vector<std::stringstream>::iterator iter = outputFileParts.begin(); iter != outputFileParts.end(); iter++)
  {
    VLOG(1) << "  " << iter->str();
  }

  LOG(DEBUG) << "open MPI file \"" << filenameStr << "\".";

  // open file
  MPI_File fileHandle;
  MPIUtility::handleReturnValue(MPI_File_open(this->rankSubset_->mpiCommunicator(), filenameStr.c_str(),
                                              //MPI_MODE_WRONLY | MPI_MODE_CREATE | MPI_MODE_UNIQUE_OPEN,
                                              MPI_MODE_WRONLY | MPI_MODE_CREATE,
                                              MPI_INFO_NULL, &fileHandle), "MPI_File_open");

  Control::PerformanceMeasurement::start("durationParaview1DWrite");

  // write beginning of file on rank 0
  outputFilePartNo = 0;

  writeAsciiDataShared(fileHandle, ownRankNo, outputFileParts[outputFilePartNo].str());
  outputFilePartNo++;

  VLOG(1) << "get current shared file position";

  // get current file position
  MPI_Offset currentFilePosition = 0;
  MPIUtility::handleReturnValue(MPI_File_get_position_shared(fileHandle, &currentFilePosition), "MPI_File_get_position_shared");
  LOG(DEBUG) << "current shared file position: " << currentFilePosition;

  // write field variables
  // loop over field variables
  int fieldVariableNo = 0;
  for (std::vector<std::pair<std::string,int>>::iterator pointDataArrayIter = vtkPiece_.properties.pointDataArrays.begin();
       pointDataArrayIter != vtkPiece_.properties.pointDataArrays.end(); pointDataArrayIter++, fieldVariableNo++)
  {
    assert(fieldVariableValues.find(pointDataArrayIter->first) != fieldVariableValues.end());

    // write values
    bool writeFloatsAsInt = pointDataArrayIter->first == "partitioning";    // for partitioning, convert float values to integer values for output
    writeCombinedValuesVector(fileHandle, ownRankNo, fieldVariableValues[pointDataArrayIter->first], fieldVariableNo, writeFloatsAsInt);

    // write next xml constructs
    writeAsciiDataShared(fileHandle, ownRankNo, outputFileParts[outputFilePartNo].str());
    outputFilePartNo++;
  }

  // write geometry field data
  writeCombinedValuesVector(fileHandle, ownRankNo, geometryFieldValues, fieldVariableNo++);

  // write next xml constructs
  writeAsciiDataShared(fileHandle, ownRankNo, outputFileParts[outputFilePartNo].str());
  outputFilePartNo++;

  // write connectivity values
  writeCombinedValuesVector(fileHandle, ownRankNo, connectivityValues, fieldVariableNo++);

  // write next xml constructs
  writeAsciiDataShared(fileHandle, ownRankNo, outputFileParts[outputFilePartNo].str());
  outputFilePartNo++;

  // write offset values
  writeCombinedValuesVector(fileHandle, ownRankNo, offsetValues, fieldVariableNo++);

  // write next xml constructs
  writeAsciiDataShared(fileHandle, ownRankNo, outputFileParts[outputFilePartNo].str());

  /*
    int array_of_sizes[1];
    array_of_sizes[0]=numProcs;
    int array_of_subsizes[1];
    array_of_subsizes[0]=1;
    int array_of_starts[1];
    array_of_starts[0]=myId;


    MPI_Datatype accessPattern;
    MPI_Type_create_subarray(1,array_of_sizes, array_of_subsizes, array_of_starts, MPI_ORDER_C, MPI_BYTE, &accessPattern);
    MPI_Type_commit(&accessPattern);

    MPI_File_set_view(fh, 0, MPI_BYTE, accessPattern, "native", MPI_INFO_NULL);
    MPI_File_write(fh, v, size, MPI_BYTE, MPI_STATUS_IGNORE);
  */

  Control::PerformanceMeasurement::stop("durationParaview1DWrite");

  MPIUtility::handleReturnValue(MPI_File_close(&fileHandle), "MPI_File_close");
}

} // namespace
