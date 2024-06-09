#!/usr/bin/env bash

nodes=(8 16 32)
tests=(
  "examples/electrophysiology/fibers/fibers_contraction/no_precice/cuboid_muscle"
  "examples/electrophysiology/fibers/fibers_contraction/no_precice/biceps"
)

for t in "${tests[@]}"; do
  echo "cd into ${t}"
  cd "${t}" || exit 1
  for n in "${nodes[@]}"; do
    echo "running ${n} nodes"
    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_base" ./run.sh
    MPIRUN="mpirun -n ${n}" TIMING_FILENAME="timing_${n}_checkpointing" SETTINGS="../settings_checkpointing.py" ./run.sh
  done
  cd - || exit 1
done
