#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
# This script reads a .bin fiber file and extracts the seed points and writes them to a csv file.
# For .bin files.
#
# usage: extract_seed_points.py <input filename> [<output filename>]

import sys, os
import numpy as np
import struct
import datetime
import pickle
import time

input_filename = "fibers.bin"
output_filename = "{}.csv".format(input_filename)

if len(sys.argv) >= 2:
  input_filename = sys.argv[1]
  output_filename = "{}.csv".format(input_filename)
if len(sys.argv) >= 3:
  output_filename = sys.argv[2]
if len(sys.argv) <= 1:
  print("usage: extract_seed_points.py <input filename> [<output filename>]")
  quit()

print("input filename: {}\noutput filename: {}".format(input_filename, output_filename))

with open(input_filename, "rb") as infile:
  
  # parse header
  bytes_raw = infile.read(32)
  header_str = struct.unpack('32s', bytes_raw)[0]
  
  header_length_raw = infile.read(4)
  header_length = struct.unpack('i', header_length_raw)[0]
  #header_length = 32+8
  parameters = []
  for i in range(int(header_length/4) - 1):
    int_raw = infile.read(4)
    value = struct.unpack('i', int_raw)[0]
    parameters.append(value)
    
  n_fibers_total = parameters[0]
  n_points_whole_fiber = parameters[1]
  n_fibers_x = (int)(np.sqrt(parameters[0]))
  n_fibers_y = n_fibers_x
  
  if "version 2" in str(header_str):   # the version 2 has number of fibers explicitly stored and thus also allows non-square dimension of fibers
    n_fibers_x = parameters[2]
    n_fibers_y = parameters[3]
  
  print("header: {}".format(header_str))
  print("nFibersTotal:      {n_fibers} = {n_fibers_x} x {n_fibers_y}".format(n_fibers=parameters[0], n_fibers_x=n_fibers_x, n_fibers_y=n_fibers_y))
  print("nPointsWholeFiber: {}".format(parameters[1]))
  if "version 2" not in str(header_str):
    print("nBoundaryPointsXNew: {}".format(parameters[2]))
    print("nBoundaryPointsZNew: {}".format(parameters[3]))
  print("nFineGridFibers_:  {}".format(parameters[4]))
  print("nRanks:            {}".format(parameters[5]))
  print("nRanksZ:           {}".format(parameters[6]))
  print("nFibersPerRank:    {}".format(parameters[7]))
  print("date:              {:%d.%m.%Y %H:%M:%S}".format(datetime.datetime.fromtimestamp(parameters[8])))
  
  streamlines = []
  n_streamlines_valid = 0
  n_streamlines_invalid = 0
  
  try:
    f = open(output_filename,"w")
  except:
    print("Could not write to file {}.".format(output_filename))
    quit()
  
  # loop over fibers
  for streamline_no in range(n_fibers_total):
    streamline = []
    streamline_valid = True
    
    # loop over points of fiber
    for point_no in range(n_points_whole_fiber):
      point = []
      
      # parse point
      for i in range(3):
        double_raw = infile.read(8)
        value = struct.unpack('d', double_raw)[0]
        point.append(value)
        
      # check if point is valid
      if point[0] == 0.0 and point[1] == 0.0 and point[2] == 0.0:
        if streamline_valid:
          coordinate_x = streamline_no % n_fibers_x
          coordinate_y = (int)(streamline_no / n_fibers_x)
          print("Error: streamline {}, ({},{})/({},{}) is invalid ({}. point)".format(streamline_no, coordinate_x, coordinate_y, n_fibers_x, n_fibers_y, point_no))
          print("streamline so far: ",streamline[0:10])
        streamline_valid = False
      streamline.append(point)
      
    if streamline_valid:
      n_streamlines_valid += 1
    else:
      n_streamlines_invalid += 1
      streamline = []
    streamlines.append(streamline)
    
    point = streamline[int(n_points_whole_fiber/2)]
    f.write("{};{}\n".format(point[0],point[1]))
  
  print("n valid: {}, n invalid: {}".format(n_streamlines_valid, n_streamlines_invalid))         
  print("File {} written.".format(output_filename))
