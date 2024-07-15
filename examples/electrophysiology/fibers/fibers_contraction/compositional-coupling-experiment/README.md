# Replication data for section "Compositional coupling"

This directory contains the setup files and instructions to reproduce the experiments from section "Compositional coupling" of Homs-Pons et al. "Partitioned Multiphysics Simulation of an Electrophysiological Three-Tendon-Biceps Model". In addition, we provide the results of the simulation.

## 1. Data set structure

This dataset contains

- A `src/` folder with the muscle and tendon participants solvers
- A `SConscript` and `SConstruct` files, used to compile the executables using `scons`
- Setting files, one for each participant
    - muscle mechanics
    - muscle fibers
    - bottom tendon
    - top-a tendon
    - top-b tendon
- A `helper.py`, and `variables/` folder, which complement the settings file
- The precice_configuration file `precice_config.xml`
- A `start_all.sh` script to start the simulation of the four participants
- A `results/` folder with the results of the simulaton
- A `profiling.json` file with the performance results

## 2. Requirements

### 2.1 Software
To replicate the multi coupling experiment it is necessary to install OpenDiHu and preCICE. In the root README.md of this repository you can find instructions on how to build and install the software packages. 

### 2.2 Input files
Additional [input files](https://zenodo.org/records/4705982) are required to run the experiment. The input files contain the geometry and material models. 
 
Download **input.tgz** and extract the files at ` $OPENDIHU_HOME/examples/electrophysiology/input`.

# 3. Running the experiments

## 3.1 Building the solvers

OpenDiHu uses scons to build the executables. The easiest way to build the solvers is by adding the following shortcuts:

```bash
alias sr='$OPENDIHU_HOME/scripts/shortcuts/sr.sh'
alias mkorn='$OPENDIHU_HOME/scripts/shortcuts/mkorn.sh'
```

Then you can build the executables by simply running `mkorn && sr` in the current directory. This will create a `build_release/` executables with the executables. 


## 3.2 Executing the experiments

We provide a script which starts all four participants from the current directory. Thus you can just run

```bash
. start_all.sh
```

Alternatively you can execute each participant manually. For example you can start the muscle participant as follows

```bash
cd build_release
rm -r precice-run
mpirun -n 1 ./muscle_mechanics ../settings_muscle_mechanics.py ramp.py --case_name "ramp"
```

The other participants can be started similarly. All participants are executed in serial except for the muscle fibers, which were executed using 16 mpi ranks. Refer to `start_all.sh` for the details. 

## 3.3 Visualizing the results
We look at the results of the simulation using paraView. The results are located at the directory `build_release/out/ramp/`. You can change the name of the output directory modifying the input for `--case_name` in `start_all.sh`.

## 3.4 Analyzing performance

The `profiling.json` includes the performance results for a simulation of 10 ms. To analyze performance we used the [performance analysis tool from preCICE](https://precice.org/tooling-performance-analysis.html#configuration). 

You can analyze each participant separately. E.g., for the muscle mechanics participant run
```
precice-profiling analyze MuscleMechanics
```
The time spent in the solver is given by `solver.advance`.

You can also generate the `profiling.json` yourself. For that you need to run the experiments. This will create a `precice-profiling` folder, which contains the information for each participant. If you run
```
precice-profiling merge
```  

then you will obtain the `profiling.json` file. 

The provided `profiling.json` corresponds to a simulation of 10 ms. So to obtain the exact same results, you have to change the simulation end time in `precice_config.xml`. 
```    
<max-time value="10.0"/>                     

```
