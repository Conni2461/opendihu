#!/usr/bin/env bash

nodes=(8 16)
tests=(
  "examples/electrophysiology/fibers/fibers_contraction/no_precice/biceps"
)

for t in "${tests[@]}"; do
  echo "cd into ${t}"
  cd "${t}" || exit 1
  for n in "${nodes[@]}"; do
    echo "running ${n} nodes"
    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_no_output" SETTINGS="../settings_no_output.py" LOGS="logs_$(date -Iseconds)_no_output" timeout 30m ./run.sh
    rm -rf build_release/out

    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_hdf5_combined" SETTINGS="../settings_hdf5_combined.py" LOGS="logs_$(date -Iseconds)_hdf5_combined" timeout 30m ./run.sh
    du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_hdf5_combined"
    rm -rf build_release/out build_release/states

    # HDF5_PLUGIN_PATH="/import/sgs.scratch/hausersn/opendihu-dev/dependencies/hdf5volasync/install/lib/" HDF5_VOL_CONNECTOR="async under_vol=0;under_info={}" MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_hdf5_async" SETTINGS="../settings_hdf5_async.py" LOGS="logs_$(date -Iseconds)_hdf5_async" timeout 30m ./run.sh
    # du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_hdf5_async"
    # rm -rf build_release/out build_release/states

    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_hdf5_independent" SETTINGS="../settings_hdf5_independent.py" LOGS="logs_$(date -Iseconds)_hdf5_independent" timeout 30m ./run.sh
    du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_hdf5_independent"
    rm -rf build_release/out build_release/states

    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_json_combined" SETTINGS="../settings_json_combined.py" LOGS="logs_$(date -Iseconds)_json_combined" timeout 30m ./run.sh
    du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_json_combined"
    rm -rf build_release/out build_release/states

    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_json_independent" SETTINGS="../settings_json_independent.py" LOGS="logs_$(date -Iseconds)_json_independent" timeout 30m ./run.sh
    du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_json_independent"
    rm -rf build_release/out build_release/states

    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_python_json" SETTINGS="../settings_python_json.py" LOGS="logs_$(date -Iseconds)_python_json" timeout 30m ./run.sh
    du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_python_json"
    rm -rf build_release/out build_release/states

    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_python_binary" SETTINGS="../settings_python_binary.py" LOGS="logs_$(date -Iseconds)_python_binary" timeout 30m ./run.sh
    du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_python_binary"
    rm -rf build_release/out build_release/states

    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_paraview" SETTINGS="../settings_paraview.py" LOGS="logs_$(date -Iseconds)_paraview" timeout 30m ./run.sh
    du -s build_release/states/* | sort -k2 | awk 'BEGIN{printf "checkpoint,size\n"} {printf "%s,%s\n",$2,$1}' > "build_release/du_${n}_paraview"
    rm -rf build_release/out build_release/states

    zip timing_${n}.zip build_release/timing_${n}_*
    zip du_${n}.zip build_release/du_${n}_*
    zip logs_${n}.zip build_release/logs_*
    rm build_release/timing_${n}_* build_release/logs_* build_release/du_${n}_*
  done
  cd - || exit 1
done
