#include "control/diagnostic_tool/solver_structure_visualizer.h"

#include "data_management/output_connector_data.h"

//! add a solver to the diagram
template<typename FunctionSpaceType, int nComponents1, int nComponents2>
void SolverStructureVisualizer::
setOutputConnectorData(std::shared_ptr<Data::OutputConnectorData<FunctionSpaceType,nComponents1,nComponents2>> outputConnectorData, bool isFromTuple)
{
  LOG(DEBUG) << "SolverStructureVisualizer::setOutputConnectorData() nDisableCalls_: " << nDisableCalls_ << ", enabled: " << enabled_ << ", currently at \"" << currentSolver_->name << "\".";

  if (!enabled_)
    return;

  if (!outputConnectorData)
  {
    LOG(WARNING) << "In SolverStructureVisualizer::setOutputConnectorData, outputConnectorData is not set";
    return;
  }

  if (!isFromTuple)
    currentSolver_->outputSlots.clear();

  // loop over ComponentOfFieldVariable entries as variable1 in outputConnectorData
  for (int i = 0; i < outputConnectorData->variable1.size(); i++)
  {
    Data::ComponentOfFieldVariable<FunctionSpaceType,nComponents1> &entry = outputConnectorData->variable1[i];

    // add the given field variable components to outputSlots
    currentSolver_->outputSlots.emplace_back();
    currentSolver_->outputSlots.back().fieldVariableName = entry.values->name();
    currentSolver_->outputSlots.back().componentName = entry.values->componentName(entry.componentNo);
    currentSolver_->outputSlots.back().nComponents = nComponents1;
    currentSolver_->outputSlots.back().variableNo = 1;
    currentSolver_->outputSlots.back().meshDescription = entry.values->functionSpace()->getDescription();
  }

  // if the geometry is set, also add it to the list
  if (outputConnectorData->geometryField && !isFromTuple)
  {
    currentSolver_->outputSlots.emplace_back();
    currentSolver_->outputSlots.back().fieldVariableName = "(geometry)";
    currentSolver_->outputSlots.back().componentName = "(geometry)";
    currentSolver_->outputSlots.back().nComponents = 3;
    currentSolver_->outputSlots.back().variableNo = 1;
  }

  // loop over ComponentOfFieldVariable entries as variable2 in outputConnectorData
  for (int i = 0; i < outputConnectorData->variable2.size(); i++)
  {
    Data::ComponentOfFieldVariable<FunctionSpaceType,nComponents2> &entry = outputConnectorData->variable2[i];

    // add the given field variable components to outputSlots
    currentSolver_->outputSlots.emplace_back();
    currentSolver_->outputSlots.back().fieldVariableName = entry.values->name();
    currentSolver_->outputSlots.back().componentName = entry.values->componentName(entry.componentNo);
    currentSolver_->outputSlots.back().nComponents = nComponents2;
    currentSolver_->outputSlots.back().variableNo = 2;
    currentSolver_->outputSlots.back().meshDescription = entry.values->functionSpace()->getDescription();
  }

  LOG(DEBUG) << "added " << currentSolver_->outputSlots.size() << " output slots";
}

template<typename T>
void SolverStructureVisualizer::
setOutputConnectorData(std::shared_ptr<std::vector<T>> outputConnectorData, bool isFromTuple)
{
  if (!outputConnectorData)
  {
    LOG(WARNING) << "In SolverStructureVisualizer::setOutputConnectorData, outputConnectorData is not set";
    return;
  }

  // if outputConnectorData is a vector, only use the first entry
  if (!outputConnectorData->empty())
  {
    setOutputConnectorData((*outputConnectorData)[0], isFromTuple);
  }
}

template<typename OutputConnectorData1, typename OutputConnectorData2>
void SolverStructureVisualizer::
setOutputConnectorData(std::shared_ptr<std::tuple<OutputConnectorData1,OutputConnectorData2>> outputConnectorData, bool isFromTuple)
{
  currentSolver_->outputSlots.clear();
  setOutputConnectorData(std::get<0>(*outputConnectorData), true);
  setOutputConnectorData(std::get<1>(*outputConnectorData), true);
}
