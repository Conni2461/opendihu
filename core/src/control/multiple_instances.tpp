#include "control/multiple_instances.h"

#include <omp.h>
#include <sstream>

#include "data_management/multiple_instances.h"
#include "partition/partition_manager.h"
#include "utility/mpi_utility.h"

namespace Control
{

template<class TimeSteppingScheme>
MultipleInstances<TimeSteppingScheme>::
MultipleInstances(DihuContext context) :
  context_(context["MultipleInstances"]), data_(context_)
{
  specificSettings_ = context_.getPythonConfig();
  outputWriterManager_.initialize(specificSettings_);
  
  //VLOG(1) << "MultipleInstances constructor, settings: " << specificSettings_;
  
  // extract the number of instances
  nInstances_ = PythonUtility::getOptionInt(specificSettings_, "nInstances", 1, PythonUtility::Positive);
   
  // parse all instance configs 
  std::vector<PyObject *> instanceConfigs;
  
  // get the config for the first InstancesDataset instance from the list
  PyObject *instanceConfig = PythonUtility::getOptionListBegin<PyObject *>(specificSettings_, "instances");

  int i = 0;
  for(;
      !PythonUtility::getOptionListEnd(specificSettings_, "instances") && i < nInstances_; 
      PythonUtility::getOptionListNext<PyObject *>(specificSettings_, "instances", instanceConfig), i++)
  {
    instanceConfigs.push_back(instanceConfig);
    VLOG(1) << "i = " << i << ", instanceConfig = " << instanceConfig;
  }
    
  if (i < nInstances_)
  {
    LOG(ERROR) << "Could only create " << i << " instances from the given instances config python list, but nInstances = " << nInstances_;
    nInstances_ = i;
  }
    
  if (!PythonUtility::getOptionListEnd(specificSettings_, "instances"))
  {
    LOG(ERROR) << "Only " << nInstances_ << " instances were created, but more configurations are given.";
  }
  
  VLOG(1) << "MultipleInstances constructor, create Partitioning for " << nInstances_ << " instances";
  
  MPIUtility::gdbParallelDebuggingBarrier();
  
  // create all instances
  for (int instanceConfigNo = 0; instanceConfigNo < nInstances_; instanceConfigNo++)
  {
    PyObject *instanceConfig = instanceConfigs[instanceConfigNo];
   
    // extract ranks for this instance
    if (!PythonUtility::hasKey(instanceConfig, "ranks"))
    {
      LOG(ERROR) << "Instance " << instanceConfigs << " has no \"ranks\" settings.";
      continue;
    }
    else 
    {
      // extract rank list
      std::vector<int> ranks;
      PythonUtility::getOptionVector(instanceConfig, "ranks", ranks);
      
      // check if own rank is part of ranks list
      int thisRankNo = this->context_.partitionManager().rankNoCommWorld();
      bool computeOnThisRank = false;
      for (int rank : ranks)
      {
        if (rank == thisRankNo)
        {
          computeOnThisRank = true;
          break;
        }
      }
      
      if (!computeOnThisRank)
        continue;
      
      // create rank subset
      Partition::RankSubset rankSubset(ranks);
      
      // store the rank subset containing only the own rank for the mesh of the current instance
      this->context_.partitionManager().setRankSubsetForNextCreatedMesh(rankSubset);
      
      VLOG(1) << "create sub context";
      instances_.emplace_back(context_.createSubContext(instanceConfig));
    
    }
  }
  
}

template<class TimeSteppingScheme>
void MultipleInstances<TimeSteppingScheme>::
advanceTimeSpan()
{
  // This method advances the simulation by the specified time span. It will be needed when this MultipleInstances object is part of a parent control element, like a coupling to 3D model.
  for (int i = 0; i < partition_->localSize(); i++)
  {
    instances_[i].advanceTimeSpan();
  }
}

template<class TimeSteppingScheme>
void MultipleInstances<TimeSteppingScheme>::
setTimeSpan(double startTime, double endTime)
{
  for (int i = 0; i < partition_->localSize(); i++)
  {
    instances_[i].setTimeSpan(startTime, endTime);
  }
}

template<class TimeSteppingScheme>
void MultipleInstances<TimeSteppingScheme>::
initialize()
{
  for (int i = 0; i < partition_->localSize(); i++)
  {
    instances_[i].initialize();
  }
  
  data_.setInstancesData(instances_);
}

template<class TimeSteppingScheme>
void MultipleInstances<TimeSteppingScheme>::
run()
{
  initialize();
 
  //#pragma omp parallel for // does not work with the python interpreter
  for (int i = 0; i < partition_->localSize(); i++)
  {
    if (omp_get_thread_num() == 0)
    {
      std::stringstream msg;
      msg << omp_get_thread_num() << ": running " << partition_->localSize() << " instances with " << omp_get_num_threads() << " OpenMP threads";
      LOG(INFO) << msg.str();
    }
    
    //instances_[i].reset();
    instances_[i].run();
  }
  
  this->outputWriterManager_.writeOutput(this->data_);
}

template<class TimeSteppingScheme>
bool MultipleInstances<TimeSteppingScheme>::
knowsMeshType()
{
  // This is a dummy method that is currently not used, it is only important if we want to map between multiple data sets.
  assert(nInstances_ > 0);
  return instances_[0].knowsMeshType();
}

template<class TimeSteppingScheme>
Vec &MultipleInstances<TimeSteppingScheme>::
solution()
{
  assert(nInstances_ > 0);
  return instances_[0].solution();
}

};