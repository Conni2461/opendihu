#!/usr/bin/env bash

MPIRUN="${MPIRUN:-}"
EXE=./biceps_contraction_Fv
SETTINGS="${SETTINGS:-../settings_biceps_contraction.py}"
VAR="ramp_Fv.py"

if [[ "$1" == "-d" ]]; then
  (cd ../../../../../../ && make debug_without_tests) && \
    python ../../../../../../dependencies/scons/scons.py BUILD_TYPE=DEBUG -j 4 && \
    (cd build_debug && ${MPIRUN} "${EXE}" "${SETTINGS}" "${VAR}" 2>&1 | tee logs.txt)
else
  (cd ../../../../../../ && make release_without_tests) && \
    python ../../../../../../dependencies/scons/scons.py BUILD_TYPE=RELEASE -j 4 && \
    (cd build_release && ${MPIRUN} "${EXE}" "${SETTINGS}" "${VAR}" 2>&1 | tee logs.txt)
fi
