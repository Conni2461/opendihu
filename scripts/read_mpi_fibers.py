#!/usr/bin/env ../../../dependencies/python/install/bin/python3
# -*- coding: utf-8 -*-
#
# This scripts reads a fibers.bin file which is generated by opendihu parallel_fiber_estimation. It outputs a stl file and a pickle file containing all fibers.
#
# usage: ./read_mpi_fibers.py [<input filename> [<mesh output filename> [<pickle output filename]]]

import sys, os
import numpy as np
import struct
import stl
from stl import mesh
import datetime
import pickle

input_filename = "fibers.bin"

if len(sys.argv) >= 2:
  input_filename = sys.argv[1]

output_filename = "{}.stl".format(input_filename)
  
if len(sys.argv) >= 3:
  output_filename = sys.argv[2]
output_filename2 = output_filename+"_"
  
pickle_output_filename = "{}.pickle".format(input_filename)
if len(sys.argv) == 4:
  pickle_output_filename = sys.argv[3]
pickle_output_filename2 = pickle_output_filename+"_"
  
print("{} -> {}, {}".format(input_filename, output_filename, pickle_output_filename))

svg_bottom_filename = "{}_bottom.svg".format(input_filename)
svg_center_filename = "{}_center.svg".format(input_filename)
svg_top_filename = "{}_top.svg".format(input_filename)

with open(input_filename, "rb") as infile:
  
  # parse header
  bytes_raw = infile.read(32)
  header_str = struct.unpack('32s', bytes_raw)[0]
  print("header: {}".format(header_str))
  
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
  
  print("nFibersTotal:      {n_fibers} = {n_fibers_x} x {n_fibers_x}".format(n_fibers=parameters[0], n_fibers_x=n_fibers_x))
  print("nPointsWholeFiber: {}".format(parameters[1]))
  print("nBorderPointsXNew: {}".format(parameters[2]))
  print("nBorderPointsZNew: {}".format(parameters[3]))
  print("nFineGridFibers_:  {}".format(parameters[4]))
  print("nRanks:            {}".format(parameters[5]))
  print("nRanksZ:           {}".format(parameters[6]))
  print("nFibersPerRank:    {}".format(parameters[7]))
  print("date:              {:%d.%m.%Y %H:%M:%S}".format(datetime.datetime.fromtimestamp(parameters[8])))
  
  input("Press any key to continue.")
  
  streamlines = []
  n_streamlines_valid = 0
  n_streamlines_invalid = 0
  
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
  
  print("n valid: {}, n invalid: {}".format(n_streamlines_valid, n_streamlines_invalid))
  print("output pickle to filename: {}".format(pickle_output_filename))
  with open(pickle_output_filename, 'wb') as f:
    pickle.dump(streamlines, f)
  print("done")
  
  if n_streamlines_invalid == 0:
    print("output svg files: {}, {}".format(svg_top_filename, svg_center_filename, svg_bottom_filename))
    
    paths = ["", "", ""]  # path strings for bottom, center, top
    z_index = [0, (int)(n_points_whole_fiber/2.), -1]
    
    
    # set circles at points
    # loop over grid of fibers
    for y in range(n_fibers_y):
      for x in range(n_fibers_x):
        for i in range(3):
          point0 = streamlines[y*n_fibers_x + x][z_index[i]]
    
          stroke_style = "#000000"
          if x == 0:
            stroke_style = "#0000aa"
          if x == n_fibers_x-2:
            stroke_style = "#3333ff"
          if y == 0:
            stroke_style = "#aa0000"
          if y == n_fibers_x-2:
            stroke_style = "#ff3333"
          if (y == 0 or y == n_fibers_y-1) and (x == 0 or x == n_fibers_x-1):
            stroke_style = "#00ff00"
            
          paths[i] += """
      <circle cx="{cx}" cy="{cy}" r="0.1" stroke="{stroke}" stroke-width="0.1" fill="{stroke}" />""".format(cx=point0[0], cy=point0[1], stroke=stroke_style)
    
    min_x = [None,None,None]
    max_x = [None,None,None]
    min_y = [None,None,None]
    max_y = [None,None,None]
    # loop over grid of fibers
    for y in range(n_fibers_y-1):
      for x in range(n_fibers_x-1):
        # bottom rectangle
        #  p2 p3
        #  p0 p1
        for i in range(3):
          point0 = streamlines[y*n_fibers_x + x][z_index[i]]
          point1 = streamlines[y*n_fibers_x + x+1][z_index[i]]
          point2 = streamlines[(y+1)*n_fibers_x + x][z_index[i]]
          point3 = streamlines[(y+1)*n_fibers_x + x+1][z_index[i]]
      
          if min_x[i] is None:
            min_x[i] = point0[0]
          min_x[i] = min(min_x[i], point0[0])
          min_x[i] = min(min_x[i], point1[0])
          min_x[i] = min(min_x[i], point2[0])
          min_x[i] = min(min_x[i], point3[0])
          
          if max_x[i] is None:
            max_x[i] = point0[0]
          max_x[i] = max(max_x[i], point0[0])
          max_x[i] = max(max_x[i], point1[0])
          max_x[i] = max(max_x[i], point2[0])
          max_x[i] = max(max_x[i], point3[0])
          
          if min_y[i] is None:
            min_y[i] = point0[1]
          min_y[i] = min(min_y[i], point0[1])
          min_y[i] = min(min_y[i], point1[1])
          min_y[i] = min(min_y[i], point2[1])
          min_y[i] = min(min_y[i], point3[1])
          
          if max_y[i] is None:
            max_y[i] = point0[1]
          max_y[i] = max(max_y[i], point0[1])
          max_y[i] = max(max_y[i], point1[1])
          max_y[i] = max(max_y[i], point2[1])
          max_y[i] = max(max_y[i], point3[1])
      
          stroke_style = "#000000"
          if x == 0:
            stroke_style = "#0000aa"
          if x == n_fibers_x-2:
            stroke_style = "#3333ff"
          if y == 0:
            stroke_style = "#aa0000"
          if y == n_fibers_x-2:
            stroke_style = "#ff3333"
          if (y == 0 or y == n_fibers_y-2) and (x == 0 or x == n_fibers_x-2):
            stroke_style = "#00ff00"
      
          paths[i] += """
        <!-- xy {} {}, len {}, points {} {} {} {}-->""".format(x,y,len(streamlines[y*n_fibers_x + x]), str(point0), str(point1), str(point2), str(point3))
          paths[i] += """
        <path
           style="fill:none;fill-rule:evenodd;stroke:{stroke};stroke-width:0.1;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:1;stroke-miterlimit:4;stroke-dasharray:none"
           d="m {p0x},{p0y} {d1x},{d1y} {d2x},{d2y} {d3x},{d3y} z" />
           """.format(stroke=stroke_style,p0x=point0[0], p0y=point0[1], d1x=(point1[0]-point0[0]), d1y=(point1[1]-point0[1]), d2x=(point3[0]-point1[0]), d2y=(point3[1]-point1[1]), d3x=(point2[0]-point3[0]), d3y=(point2[1]-point3[1]))
      
    svg_filenames = [svg_bottom_filename, svg_center_filename, svg_top_filename]
    for i in range(3):
      filename = svg_filenames[i]
      with open(filename, "w") as f:
        f.write("""<?xml version="1.0" encoding="UTF-8"?>
    <svg width="{lx}" height="{ly}">
      <g transform="translate({tx},{ty})">
    {paths}
      </g>
    </svg>
  """.format(paths=paths[i], tx=-min_x[i], ty=-min_y[i], lx=(max_x[i]-min_x[i]), ly=(max_y[i]-min_y[i])))
        print("wrote file {}".format(filename))
      
  # create stl file
  triangles = []
  for points in streamlines:
    previous_point = None
    
    for p in points:
      point = np.array([p[0], p[1], p[2]])
      if np.linalg.norm(point) < 1e-3:
        continue
      if previous_point is not None:
        triangles.append([previous_point, point, 0.5*(previous_point+point)])
      previous_point = point

  #---------------------------------------
  # Create the mesh
  out_mesh = mesh.Mesh(np.zeros(len(triangles), dtype=mesh.Mesh.dtype))
  for i, f in enumerate(triangles):
    out_mesh.vectors[i] = f
  #out_mesh.update_normals()

  out_mesh.save(output_filename)
  print("saved {} triangles to \"{}\"".format(len(triangles),output_filename))
  
  print("postprocessing where fibres with too high distance to neighbouring fibers are removed (may take long)")
  input("Press any key to continue.")
  
  
  # postprocess streamlines
  invalid_streamlines = []
  n_fibers_x = (int)(np.sqrt(n_fibers_total))
  for j in range(0,n_fibers_x):
    for i in range(0,n_fibers_x):
      fiber_no = j*n_fibers_x + i
      
      fiber_no_0minus = None
      fiber_no_0plus = None
      fiber_no_1minus = None
      fiber_no_1plus = None
      if i > 0:
        fiber_no_0minus = j*n_fibers_x + i-1
      if i < n_fibers_x-1:
        fiber_no_0plus = j*n_fibers_x + i+1
      if j > 0:
        fiber_no_1minus = (j-1)*n_fibers_x + i
      if j < n_fibers_x-1:
        fiber_no_1plus = (j+1)*n_fibers_x + i
      
      average_distance = 0
      max_distance = 0
      n_points = 0
      for point_no in range(n_points_whole_fiber):
        if len(streamlines[fiber_no]) > point_no:
          if fiber_no_0minus is not None:
            if len(streamlines[fiber_no_0minus]) > point_no:
              #average_distance += np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_0minus][point_no]))
              max_distance = max(max_distance, np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_0minus][point_no])))
              n_points += 1
          if fiber_no_1minus is not None:
            if len(streamlines[fiber_no_1minus]) > point_no:
              #average_distance += np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_1minus][point_no]))
              max_distance = max(max_distance, np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_1minus][point_no])))
              n_points += 1
          if fiber_no_0plus is not None:
            if len(streamlines[fiber_no_0plus]) > point_no:
              #average_distance += np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_0plus][point_no]))
              max_distance = max(max_distance, np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_0plus][point_no])))
              n_points += 1
          if fiber_no_1plus is not None:
            if len(streamlines[fiber_no_1plus]) > point_no:
              #average_distance += np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_1plus][point_no]))
              max_distance = max(max_distance, np.linalg.norm(np.array(streamlines[fiber_no][point_no]) - np.array(streamlines[fiber_no_1plus][point_no])))
              n_points += 1
      #if n_points > 0:
        #average_distance /= n_points
      print (" fiber {},{}, average_distance: {}, n_points: {}, max_distance: {}".format(i,j,average_distance, n_points, max_distance))
      if max_distance > 10:
        invalid_streamlines.append(fiber_no)
  
  for invalid_streamline_no in invalid_streamlines:
    streamlines[invalid_streamline_no] = []


  print("output other pickle to filename: {}".format(pickle_output_filename2))
  with open(pickle_output_filename2, 'wb') as f:
    pickle.dump(streamlines, f)
  
  #streamlines = [streamlines[5]]
  #print(streamlines[0])
  
  triangles = []
  for points in streamlines:
    previous_point = None
    
    for p in points:
      point = np.array([p[0], p[1], p[2]])
      if np.linalg.norm(point) < 1e-3:
        continue
      if previous_point is not None:
        triangles.append([previous_point, point, 0.5*(previous_point+point)])
      previous_point = point

  #---------------------------------------
  # Create the mesh
  out_mesh = mesh.Mesh(np.zeros(len(triangles), dtype=mesh.Mesh.dtype))
  for i, f in enumerate(triangles):
    out_mesh.vectors[i] = f
  #out_mesh.update_normals()

  out_mesh.save(output_filename2)
  print("saved {} triangles to \"{}\"".format(len(triangles),output_filename2))
