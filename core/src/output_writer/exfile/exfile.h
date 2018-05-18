#pragma once

#include <Python.h>  // has to be the first included header
#include <iostream>
#include <vector>

#include "control/types.h"
#include "output_writer/generic.h"

namespace OutputWriter
{   

class Exfile : public Generic
{
public:
 
  //! constructor
  Exfile(PyObject *specificSettings);
 
  //! write out solution to given filename, if timeStepNo is not -1, this value will be part of the filename
  template<typename DataType>
  void write(DataType &data, int timeStepNo = -1, double currentTime = -1);
  
private:
 
  //! output a cmgui "visualization.com" file that references the exfiles and can be used by Cmgui
  void outputComFile();
  
  std::vector<std::string> filenames_;   ///< the filenames without suffix of all previously output exelem files
};

};  // namespace

#include "output_writer/exfile/exfile.tpp"
