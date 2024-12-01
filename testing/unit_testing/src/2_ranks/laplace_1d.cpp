#include <Python.h> // this has to be the first included header

#include <iostream>
#include <cstdlib>
#include <fstream>

#include "gtest/gtest.h"
#include "arg.h"
#include "opendihu.h"
#include "../utility.h"
#include "input_reader/hdf5/full_dataset.h"

TEST(LaplaceTest, Structured1DLinear) {
  std::string pythonConfig = R"(
# Laplace 1D
n = 5

# boundary conditions
bc = {}
bc[0] = 1.0
bc[-1] = 0.0

config = {
  "FiniteElementMethod": {
    "inputMeshIsGlobal": True,
    "nElements": n,
    "physicalExtent": 4.0,
    "dirichletBoundaryConditions": bc,
    "relativeTolerance": 1e-15,
    "OutputWriter" : [
      {"format": "Paraview", "filename": "out4", "outputInterval": 1, "binary": False, "fixedFormat": True, "combineFiles": False, "onlyNodalValues": True},
      {"format": "PythonFile", "filename": "out4", "outputInterval": 1, "binary": False, "onlyNodalValues": True},
      {"format": "HDF5", "filename": "out4", "outputInterval": 1, "combineFiles": False},
    ]
  }
}
)";

  DihuContext settings(argc, argv, pythonConfig);

  SpatialDiscretization::FiniteElementMethod<
      Mesh::StructuredDeformableOfDimension<1>,
      BasisFunction::LagrangeOfOrder<>, Quadrature::Gauss<2>,
      Equation::Static::Laplace>
      problem(settings);

  problem.run();

  LOG(INFO) << "wait 1 s";
  std::this_thread::sleep_for(std::chrono::milliseconds(
      1000)); // pause execution, such that output files can be closed

  std::string referenceOutput0 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [3], "
      "\"beginNodeGlobalNatural\": [0], \"hasFullNumberOfNodes\": [false], "
      "\"basisFunction\": \"Lagrange\", \"basisOrder\": 1, "
      "\"onlyNodalValues\": true, \"nRanks\": 2, \"ownRankNo\": 0, \"data\": "
      "[{\"name\": \"geometry\", \"components\": [{\"name\": \"x\", "
      "\"values\": [0.0, 0.8, 1.6]}, {\"name\": \"y\", \"values\": [0.0, 0.0, "
      "0.0]}, {\"name\": \"z\", \"values\": [0.0, 0.0, 0.0]}]}, {\"name\": "
      "\"solution\", \"components\": [{\"name\": \"0\", \"values\": "
      "[1.0000000000000004, 0.7999999999999998, 0.6000000000000001]}]}, "
      "{\"name\": \"rightHandSide\", \"components\": [{\"name\": \"0\", "
      "\"values\": [1.0, -1.25, 0.0]}]}, {\"name\": \"-rhsNeumannBC\", "
      "\"components\": [{\"name\": \"0\", \"values\": [0.0, 0.0, 0.0]}]}], "
      "\"timeStepNo\": -1, \"currentTime\": 0.0}";
  std::string referenceOutput1 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [2], "
      "\"beginNodeGlobalNatural\": [3], \"hasFullNumberOfNodes\": [true], "
      "\"basisFunction\": \"Lagrange\", \"basisOrder\": 1, "
      "\"onlyNodalValues\": true, \"nRanks\": 2, \"ownRankNo\": 1, \"data\": "
      "[{\"name\": \"geometry\", \"components\": [{\"name\": \"x\", "
      "\"values\": [2.4000000000000004, 3.2, 4.0]}, {\"name\": \"y\", "
      "\"values\": [0.0, 0.0, 0.0]}, {\"name\": \"z\", \"values\": [0.0, 0.0, "
      "0.0]}]}, {\"name\": \"solution\", \"components\": [{\"name\": \"0\", "
      "\"values\": [0.4000000000000001, 0.1999999999999999, 0.0]}]}, "
      "{\"name\": \"rightHandSide\", \"components\": [{\"name\": \"0\", "
      "\"values\": [0.0, 0.0, 0.0]}]}, {\"name\": \"-rhsNeumannBC\", "
      "\"components\": [{\"name\": \"0\", \"values\": [0.0, 0.0, 0.0]}]}], "
      "\"timeStepNo\": -1, \"currentTime\": 0.0}";

  if (settings.ownRankNo() == 0) {
    assertFileMatchesContent("out4.0.py", referenceOutput0);
    assertFileMatchesContent("out4.1.py", referenceOutput1);

    InputReader::HDF5::FullDataset r1("out4.0_p.h5");
    InputReader::HDF5::FullDataset r2("out4.1_p.h5");
    {
      ASSERT_TRUE(r1.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r1.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r1.readDoubleVector("geometry", geometry);
      r1.readDoubleVector("solution", solution);
      compareArray(geometry, {0, 0, 0, 0.8, 0, 0, 1.6, 0, 0, 2.4, 0, 0});
      compareArray(solution, {1.0, 0.8, 0.6, 0.4});
    }
    {
      ASSERT_TRUE(r2.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r2.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r2.readDoubleVector("geometry", geometry);
      r2.readDoubleVector("solution", solution);
      compareArray(geometry, {2.4, 0, 0, 3.2, 0, 0, 4.0, 0, 0});
      compareArray(solution, {0.4, 0.2, 0.0});
    }
  }

  nFails += ::testing::Test::HasFailure();
}

TEST(LaplaceTest, Structured1DQuadratic) {
  std::string pythonConfig = R"(
# Laplace 1D
n = 5

# boundary conditions
bc = {}
bc[0] = 1.0
bc[-1] = 0.0

config = {
  "FiniteElementMethod": {
    "inputMeshIsGlobal": True,
    "nElements": n,
    "physicalExtent": 4.0,
    "dirichletBoundaryConditions": bc,
    "relativeTolerance": 1e-15,
    "OutputWriter" : [
      {"format": "Paraview", "filename": "out5", "outputInterval": 1, "binary": False},
      {"format": "PythonFile", "filename": "out5", "outputInterval": 1, "binary": False},
      {"format": "HDF5", "filename": "out5", "outputInterval": 1, "combineFiles": False},
    ]
  }
}
)";

  DihuContext settings(argc, argv, pythonConfig);

  SpatialDiscretization::FiniteElementMethod<
      Mesh::StructuredDeformableOfDimension<1>,
      BasisFunction::LagrangeOfOrder<2>, Quadrature::Gauss<2>,
      Equation::Static::Laplace>
      problem(settings);

  problem.run();

  LOG(INFO) << "wait 2 s";
  std::this_thread::sleep_for(std::chrono::milliseconds(
      2000)); // pause execution, such that output files can be closed

  std::string referenceOutput0 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [3], "
      "\"beginNodeGlobalNatural\": [0], \"hasFullNumberOfNodes\": [false], "
      "\"basisFunction\": \"Lagrange\", \"basisOrder\": 2, "
      "\"onlyNodalValues\": true, \"nRanks\": 2, \"ownRankNo\": 0, \"data\": "
      "[{\"name\": \"geometry\", \"components\": [{\"name\": \"x\", "
      "\"values\": [0.0, 0.4, 0.8, 1.2000000000000002, 1.6, 2.0]}, {\"name\": "
      "\"y\", \"values\": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}, {\"name\": \"z\", "
      "\"values\": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}]}, {\"name\": \"solution\", "
      "\"components\": [{\"name\": \"0\", \"values\": [1.0000000000000004, "
      "0.8999999999999994, 0.799999999999999, 0.699999999999998, "
      "0.5999999999999972, 0.49999999999999767]}]}, {\"name\": "
      "\"rightHandSide\", \"components\": [{\"name\": \"0\", \"values\": [1.0, "
      "-3.333333333333333, 0.41666666666666674, 0.0, 0.0, 0.0]}]}, {\"name\": "
      "\"-rhsNeumannBC\", \"components\": [{\"name\": \"0\", \"values\": [0.0, "
      "0.0, 0.0, 0.0, 0.0, 0.0]}]}], \"timeStepNo\": -1, \"currentTime\": 0.0}";
  std::string referenceOutput1 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [2], "
      "\"beginNodeGlobalNatural\": [6], \"hasFullNumberOfNodes\": [true], "
      "\"basisFunction\": \"Lagrange\", \"basisOrder\": 2, "
      "\"onlyNodalValues\": true, \"nRanks\": 2, \"ownRankNo\": 1, \"data\": "
      "[{\"name\": \"geometry\", \"components\": [{\"name\": \"x\", "
      "\"values\": [2.4000000000000004, 2.8000000000000003, 3.2, 3.6, 4.0]}, "
      "{\"name\": \"y\", \"values\": [0.0, 0.0, 0.0, 0.0, 0.0]}, {\"name\": "
      "\"z\", \"values\": [0.0, 0.0, 0.0, 0.0, 0.0]}]}, {\"name\": "
      "\"solution\", \"components\": [{\"name\": \"0\", \"values\": "
      "[0.39999999999999736, 0.2999999999999976, 0.19999999999999823, "
      "0.09999999999999895, 0.0]}]}, {\"name\": \"rightHandSide\", "
      "\"components\": [{\"name\": \"0\", \"values\": [0.0, 0.0, 0.0, 0.0, "
      "0.0]}]}, {\"name\": \"-rhsNeumannBC\", \"components\": [{\"name\": "
      "\"0\", \"values\": [0.0, 0.0, 0.0, 0.0, 0.0]}]}], \"timeStepNo\": -1, "
      "\"currentTime\": 0.0}";

  if (settings.ownRankNo() == 0) {
    assertFileMatchesContent("out5.0.py", referenceOutput0);
    assertFileMatchesContent("out5.1.py", referenceOutput1);

    InputReader::HDF5::FullDataset r1("out5.0_p.h5");
    InputReader::HDF5::FullDataset r2("out5.1_p.h5");
    {
      ASSERT_TRUE(r1.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r1.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r1.readDoubleVector("geometry", geometry);
      r1.readDoubleVector("solution", solution);
      compareArray(geometry, {0, 0,   0, 0.4, 0,   0, 0.8, 0,   0, 1.2, 0,
                              0, 1.6, 0, 0,   2.0, 0, 0,   2.4, 0, 0});
      compareArray(solution,
                   {0.99999999999999822, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4});
    }
    {
      ASSERT_TRUE(r2.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r2.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r2.readDoubleVector("geometry", geometry);
      r2.readDoubleVector("solution", solution);
      compareArray(geometry,
                   {2.4, 0, 0, 2.8, 0, 0, 3.2, 0, 0, 3.6, 0, 0, 4.0, 0, 0});
      compareArray(solution, {0.4, 0.3, 0.2, 0.1, 0});
    }
  }

  nFails += ::testing::Test::HasFailure();
}

TEST(LaplaceTest, Structured1DHermite1) {
  std::string pythonConfig = R"(
# Laplace 1D
n = 5

# boundary conditions
bc = {}
bc[0] = 1.0
bc[-2] = 0.0

config = {
  "FiniteElementMethod": {
    "inputMeshIsGlobal": True,
    "nElements": n,
    "physicalExtent": 4.0,
    "dirichletBoundaryConditions": bc,
    "relativeTolerance": 1e-15,
    "OutputWriter" : [
      {"format": "Paraview", "filename": "out6", "outputInterval": 1, "binary": False},
      {"format": "PythonFile", "filename": "out6", "outputInterval": 1, "binary": False, "onlyNodalValues": True},
      {"format": "HDF5", "filename": "out6", "outputInterval": 1, "combineFiles": False},
    ]
  }
}
)";

  DihuContext settings(argc, argv, pythonConfig);

  SpatialDiscretization::FiniteElementMethod<
      Mesh::StructuredDeformableOfDimension<1>, BasisFunction::Hermite,
      Quadrature::Gauss<3>, Equation::Static::Laplace>
      problem(settings);

  problem.run();

  LOG(INFO) << "wait 2 s";
  std::this_thread::sleep_for(std::chrono::milliseconds(
      2000)); // pause execution, such that output files can be closed

  std::string referenceOutput0 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [3], "
      "\"beginNodeGlobalNatural\": [0], \"hasFullNumberOfNodes\": [false], "
      "\"basisFunction\": \"Hermite\", \"basisOrder\": 3, \"onlyNodalValues\": "
      "true, \"nRanks\": 2, \"ownRankNo\": 0, \"data\": [{\"name\": "
      "\"geometry\", \"components\": [{\"name\": \"x\", \"values\": [0.0, 0.8, "
      "1.6]}, {\"name\": \"y\", \"values\": [0.0, 0.0, 0.0]}, {\"name\": "
      "\"z\", \"values\": [0.0, 0.0, 0.0]}]}, {\"name\": \"solution\", "
      "\"components\": [{\"name\": \"0\", \"values\": [1.0000000000000002, "
      "0.8000000000000003, 0.5999999999999999]}]}, {\"name\": "
      "\"rightHandSide\", \"components\": [{\"name\": \"0\", \"values\": [1.0, "
      "-1.4999999999999993, 0.0]}]}, {\"name\": \"-rhsNeumannBC\", "
      "\"components\": [{\"name\": \"0\", \"values\": [0.0, 0.0, 0.0]}]}], "
      "\"timeStepNo\": -1, \"currentTime\": 0.0}";
  std::string referenceOutput1 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [2], "
      "\"beginNodeGlobalNatural\": [3], \"hasFullNumberOfNodes\": [true], "
      "\"basisFunction\": \"Hermite\", \"basisOrder\": 3, \"onlyNodalValues\": "
      "true, \"nRanks\": 2, \"ownRankNo\": 1, \"data\": [{\"name\": "
      "\"geometry\", \"components\": [{\"name\": \"x\", \"values\": "
      "[2.4000000000000004, 3.2, 4.0]}, {\"name\": \"y\", \"values\": [0.0, "
      "0.0, 0.0]}, {\"name\": \"z\", \"values\": [0.0, 0.0, 0.0]}]}, "
      "{\"name\": \"solution\", \"components\": [{\"name\": \"0\", \"values\": "
      "[0.3999999999999998, 0.1999999999999998, 0.0]}]}, {\"name\": "
      "\"rightHandSide\", \"components\": [{\"name\": \"0\", \"values\": [0.0, "
      "0.0, 0.0]}]}, {\"name\": \"-rhsNeumannBC\", \"components\": [{\"name\": "
      "\"0\", \"values\": [0.0, 0.0, 0.0]}]}], \"timeStepNo\": -1, "
      "\"currentTime\": 0.0}";

  if (settings.ownRankNo() == 0) {
    assertFileMatchesContent("out6.0.py", referenceOutput0);
    assertFileMatchesContent("out6.1.py", referenceOutput1);

    InputReader::HDF5::FullDataset r1("out6.0_p.h5");
    InputReader::HDF5::FullDataset r2("out6.1_p.h5");
    {
      ASSERT_TRUE(r1.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r1.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r1.readDoubleVector("geometry", geometry);
      r1.readDoubleVector("solution", solution);
      compareArray(geometry, {0, 0, 0, 0.8, 0, 0, 1.6, 0, 0, 2.4, 0, 0});
      compareArray(solution, {0.99999999999999944, 0.8, 0.6, 0.4});
    }
    {
      ASSERT_TRUE(r2.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r2.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r2.readDoubleVector("geometry", geometry);
      r2.readDoubleVector("solution", solution);
      compareArray(geometry, {2.4, 0, 0, 3.2, 0, 0, 4, 0, 0});
      compareArray(solution, {0.4, 0.2, 0});
    }
  }

  nFails += ::testing::Test::HasFailure();
}

TEST(LaplaceTest, Structured1DHermite2) {
  std::string pythonConfig = R"(
# Laplace 1D
n = 5

# boundary conditions
bc = {}
bc[1] = -1/4.
bc[-2] = 0.0

config = {
  "FiniteElementMethod": {
    "inputMeshIsGlobal": True,
    "nElements": n,
    "physicalExtent": 4.0,
    "dirichletBoundaryConditions": bc,
    "relativeTolerance": 1e-15,
    "solverType": "gmres",
    "preconditionerType": "sor",
    "OutputWriter" : [
      {"format": "Paraview", "filename": "out7", "outputInterval": 1, "binary": False},
      {"format": "PythonFile", "filename": "out7", "outputInterval": 1, "binary": False, "onlyNodalValues": False},
      {"format": "HDF5", "filename": "out7", "outputInterval": 1, "combineFiles": False},
    ]
  }
}
)";

  DihuContext settings(argc, argv, pythonConfig);

  SpatialDiscretization::FiniteElementMethod<
      Mesh::StructuredDeformableOfDimension<1>, BasisFunction::Hermite,
      Quadrature::Gauss<3>, Equation::Static::Laplace>
      problem(settings);

  problem.run();

  std::string referenceOutput0 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [3], "
      "\"beginNodeGlobalNatural\": [0], \"hasFullNumberOfNodes\": [false], "
      "\"basisFunction\": \"Hermite\", \"basisOrder\": 3, \"onlyNodalValues\": "
      "false, \"nRanks\": 2, \"ownRankNo\": 0, \"data\": [{\"name\": "
      "\"geometry\", \"components\": [{\"name\": \"x\", \"values\": [0.0, 0.0, "
      "0.8, 0.0, 1.6, 0.0]}, {\"name\": \"y\", \"values\": [0.0, 0.0, 0.0, "
      "0.0, 0.0, 0.0]}, {\"name\": \"z\", \"values\": [0.0, 0.0, 0.0, 0.0, "
      "0.0, 0.0]}]}, {\"name\": \"solution\", \"components\": [{\"name\": "
      "\"0\", \"values\": [-1.731137919321699e-17, -0.24999999999999994, "
      "-2.2431623986420583e-18, -0.05628740946730341, 1.3321963259718046e-18, "
      "-0.01267457751408284]}]}, {\"name\": \"rightHandSide\", \"components\": "
      "[{\"name\": \"0\", \"values\": [1.3877787807814457e-17, -0.25, "
      "-1.3877787807814457e-17, 0.026041666666666668, 0.0, 0.0]}]}, {\"name\": "
      "\"-rhsNeumannBC\", \"components\": [{\"name\": \"0\", \"values\": [0.0, "
      "0.0, 0.0, 0.0, 0.0, 0.0]}]}], \"timeStepNo\": -1, \"currentTime\": 0.0}";
  std::string referenceOutput1 =
      "{\"meshType\": \"StructuredDeformable\", \"dimension\": 1, "
      "\"nElementsGlobal\": [5], \"nElementsLocal\": [2], "
      "\"beginNodeGlobalNatural\": [3], \"hasFullNumberOfNodes\": [true], "
      "\"basisFunction\": \"Hermite\", \"basisOrder\": 3, \"onlyNodalValues\": "
      "false, \"nRanks\": 2, \"ownRankNo\": 1, \"data\": [{\"name\": "
      "\"geometry\", \"components\": [{\"name\": \"x\", \"values\": "
      "[2.4000000000000004, 0.0, 3.2, 0.0, 4.0, 0.0]}, {\"name\": \"y\", "
      "\"values\": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}, {\"name\": \"z\", "
      "\"values\": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}]}, {\"name\": \"solution\", "
      "\"components\": [{\"name\": \"0\", \"values\": [1.2293431050318027e-18, "
      "-0.002860618931749887, 1.0301064917240598e-18, -0.000674977500749973, "
      "0.0, -0.0002892760717499882]}]}, {\"name\": \"rightHandSide\", "
      "\"components\": [{\"name\": \"0\", \"values\": [0.0, 0.0, 0.0, 0.0, "
      "0.0, 0.0]}]}, {\"name\": \"-rhsNeumannBC\", \"components\": [{\"name\": "
      "\"0\", \"values\": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}]}], \"timeStepNo\": "
      "-1, \"currentTime\": 0.0}";

  if (settings.ownRankNo() == 0) {
    assertFileMatchesContent("out7.0.py", referenceOutput0);
    assertFileMatchesContent("out7.1.py", referenceOutput1);

    InputReader::HDF5::FullDataset r1("out7.0_p.h5");
    InputReader::HDF5::FullDataset r2("out7.1_p.h5");
    {
      ASSERT_TRUE(r1.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r1.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r1.readDoubleVector("geometry", geometry);
      r1.readDoubleVector("solution", solution);
      compareArray(geometry, {0, 0, 0, 0.8, 0, 0, 1.6, 0, 0, 2.4, 0, 0});
      compareArray(solution, {0.029462781246902558, 0.00505501, 0.000867281,
                              0.000148677});
    }
    {
      ASSERT_TRUE(r2.hasAttribute("timeStepNo"));
      ASSERT_TRUE(r2.hasAttribute("currentTime"));
      std::vector<double> geometry, solution;
      r2.readDoubleVector("geometry", geometry);
      r2.readDoubleVector("solution", solution);
      compareArray(geometry, {2.4, 0, 0, 3.2, 0, 0, 4, 0, 0});
      compareArray(solution, {0.000148677, 2.47795e-05, 0});
    }
  }

  nFails += ::testing::Test::HasFailure();
}
