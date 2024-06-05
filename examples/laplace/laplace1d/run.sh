#!/usr/bin/env bash

(cd ../../../ && make debug_without_tests) && \
  python ../../../dependencies/scons/scons.py BUILD_TYPE=DEBUG -j 4 && \
  (cd build_debug && mpirun -n 1 ./laplace_linear ../settings_linear_quadratic_dirichlet.py 2>&1 | tee logs.txt) && \
  nix-shell -p hdf5 --run 'h5dump build_debug/out/p_c.h5'
