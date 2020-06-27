#pragma once

#include <Python.h>  // has to be the first included header

#include "interfaces/runnable.h"
#include "time_stepping_scheme/00_time_stepping_scheme.h"
#include "data_management/specialized_solver/dummy.h"

/** A dummy solver that does nothing.
  */
class Dummy :
  public Runnable,
  public TimeSteppingScheme::TimeSteppingScheme
{
public:
  //! make the FunctionSpace available
  typedef FunctionSpace::Generic FunctionSpace;

  //! define the type of the data object
  typedef ::Data::Dummy<FunctionSpace> Data;

  //! Define the type of data that will be transferred between solvers when there is a coupling scheme.
  typedef typename Data::OutputConnectorDataType OutputConnectorDataType;

  //! constructor, gets the DihuContext object which contains all python settings
  Dummy(DihuContext context);

  //! advance simulation by the given time span [startTime_, endTime_]
  void advanceTimeSpan();

  //! initialize time span from specificSettings_
  void initialize();

  //! run solution process, this first calls initialize() and then run()
  void run();

  //! reset state of this object, such that a new initialize() is necessary ("uninitialize")
  void reset();

  //! return the data object of the timestepping scheme, with the call to this method the output writers get the data to create their output files
  Data &data();

  //! Get the data that will be transferred in the operator splitting or coupling to the other term of the splitting/coupling.
  //! the transfer is done by the output_connector_data_transfer class
  std::shared_ptr<OutputConnectorDataType> getOutputConnectorData();

protected:

  Data data_;   //< the data object that provides the getOutputConnectorData function
};

