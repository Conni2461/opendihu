#!/usr/bin/env bash

buildCmd="release_without_tests"
buildType="Release"
buildDir="build_release"

if [[ "$1" = "-d" ]]; then
  buildCmd="debug_without_tests"
  buildType="Debug"
  buildDir="build_debug"
fi

(cd ../../../../../ && make "${buildCmd}")
(python3 ../../../../../dependencies/scons/scons.py "BUILD_TYPE=${buildType}" -j 32)

(
  cd "${buildDir}" || exit
  rm -rf precice-run
  rm -rf precice-profiling

  dnow=$(date -Iseconds)

  mpirun -n 16 ./muscle_mechanics ../settings_muscle_mechanics.py ramp.py --case_name "ramp" 2>&1 | tee "muscle_mechanics_${dnow}.log" &
  mpirun -n 16 ./muscle_fibers ../settings_muscle_fibers.py ramp.py --case_name "ramp" 2>&1 | tee "muscle_fibers_${dnow}.log" &
  mpirun -n 1 ./tendon ../settings_tendon_bottom.py ramp.py --case_name "ramp" 2>&1 | tee "tendon_bottom_${dnow}.log" &
  mpirun -n 1 ./tendon_linear ../settings_tendon_top_a.py ramp.py --case_name "ramp" 2>&1 | tee "tendon_top_a_${dnow}.log" &
  mpirun -n 1 ./tendon_linear ../settings_tendon_top_b.py ramp.py --case_name "ramp" 2>&1 | tee "tendon_top_b_${dnow}.log" &
  wait

  mv precice-profiling "precice-profiling-${dnow}"
)
