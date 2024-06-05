#!/usr/bin/env bash

MPIRUN="${MPIRUN:-}"
EXE=./my_new_dynamic_solver
SETTINGS="${SETTINGS:-settings_my_dynamic_solver.py}"

N=8

if [[ "$1" == "-d" ]]; then
  (cd ../../../../ && make debug_without_tests) && \
    python ../../../../dependencies/scons/scons.py BUILD_TYPE=DEBUG -j "${N}" && \
    (cd build_debug && ${MPIRUN} "${EXE}" "../${SETTINGS}" 2>&1 | tee logs.txt)
else
  (cd ../../../../ && make release_without_tests) && \
    python ../../../../dependencies/scons/scons.py BUILD_TYPE=RELEASE -j "${N}" && \
    (cd build_release && ${MPIRUN} "${EXE}" "../${SETTINGS}" 2>&1 | tee logs.txt)
fi
