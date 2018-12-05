#pragma once

#include <petscmat.h>

#include "control/dihu_context.h"
#include "data_management/data.h"
#include "data_management/model_order_reduction.h"
#include "function_space/function_space.h"

namespace ModelOrderReduction
{

/** A class for model order reduction techniques.
 */
template<typename FullFunctionSpace>
class MORBase
{
public:
  typedef Data::ModelOrderReduction<FullFunctionSpace> DataMOR; //type of Data object
  typedef FunctionSpace::Generic GenericFunctionSpace;
  
  //! constructor
  MORBase(DihuContext context);
  
  virtual ~MORBase();
  
  //! Set the basis V as Petsc Mat
  void setBasis();
  
  //! data object for model order reduction
  DataMOR &dataMOR();
  
  virtual void initialize();
   
protected:
  //! Set the reduced system matrix, A_R=V^T A V
  virtual void setRedSysMatrix(Mat &A, Mat &A_R);
  
  std::shared_ptr<DataMOR> dataMOR_;
  int nReducedBases_;    
  int nFullBases_;///< dimension of the reduced space
  
  std::shared_ptr<GenericFunctionSpace> functionSpaceRed;
  std::shared_ptr<GenericFunctionSpace> functionSpaceFull;
  
  PythonConfig specificSettingsMOR_;    ///< python object containing the value of the python config dict with corresponding key
  bool initialized_;
};

}  // namespace


#include "model_order_reduction/mor.tpp"
