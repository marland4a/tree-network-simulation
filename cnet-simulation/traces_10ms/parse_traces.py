#!/usr/bin/env python3

###############################################
#
# Script to parse traces in the same directory (filename trace_*).
# It generates the files traces_linear.csv and traces_tree.csv
# each containing a table of client_number x time_per_client.
#
###############################################

import sys
import os
import re
import pandas
import math


# Maximum number of nodes. Needed for pandas
MAXIMUM_NODES = 8192


def extract_times(filename: str) -> "dict[int, int]":
  """
  Extract a dict of {node_number_dest: reception_time}
  from the given file.
  The reception time is converted to float seconds
  """
  with open(filename, 'r') as f:
    result = dict()
    for l in f:
      line = l.rstrip().lstrip()    # Remove leading and trailing whitespace, tab and newline
      match = re.match("Node ([0-9]*) received packet from ([0-9]*) after ([0-9]*.[-0-9]*) secs", line)
      if match is not None:
        #print(match.groups())
        packet_dest = int(match.groups()[0])
        packet_src = int(match.groups()[1])     # Unused for now
        packet_time = match.groups()[2]
        # Convert time
        packet_time_sec = int(packet_time.split('.')[0])
        packet_time_usec = int(packet_time.split('.')[1])
        packet_time_sec_float = packet_time_sec + packet_time_usec / 1000000
        # Keep the shortest time if packet was delivered multiple times
        if(result.get(packet_dest) is None or result.get(packet_dest) > packet_time_sec_float):
          result[packet_dest] = packet_time_sec_float
        elif packet_dest != '0':           # Node0 is the central router which obviously receives packets more often
          print("Note: Packet to", packet_dest, "was already delivered faster. Skipping")
        #print(packet_dest, packet_time_sec_float)
        if(packet_time_sec_float.is_integer()):
          print(line)
  return result

def generate_first_column() -> "list[int]":
  """
  Generate a list of column labels for the first column.
  Log(2) numbers are used.
  It is currently limited to 8192 clients
  """
  result = []
  start = 2
  end = 8192
  while(start <= end):
    result.append(start)
    start = start * 2
  return result

def fill_result_dict(d):
  """
  Fill the result dict if less than MAXIMUM_NODES nodes
  were present in the parsed trace.
  This is required for inserting the column into the pandas
  DataFrame.
  """
  for i in range(0, MAXIMUM_NODES+1):
    if d.get(i) is None:
      d[i] = ''

### MAIN ###
pd_linear = pandas.DataFrame()
#pd_linear.insert(0, "LINEAR:\nReceiving client \\ Number of connected clients", range(0,8193))
pd_tree = pandas.DataFrame()
#pd_tree.insert(0, "TREE:\nReceiving client \\ Number of connected clients", generate_first_column())

# Generate for linear
for file in os.listdir():
  filename = os.fsdecode(file)
  match = re.match("trace_linear_([0-9]*)clients", filename)
  if match is not None:
    print("linear:", match.groups()[0])
    result = extract_times(filename)
    fill_result_dict(result)
    #result_values = [i[1] for i in sorted(result.items())]
    #result_values.extend([''] * (MAXIMUM_NODES - len(result.values())))
    #print(result_values)
    #pd_linear.insert(len(pd_linear.columns), int(match.groups()[0]), result.items(),
    #                 allow_duplicates=False)
    pd_append = pandas.DataFrame(index=result.keys(), data=result.values(),
                                 columns=[int(match.groups()[0])])
    pd_linear[int(match.groups()[0])] = pd_append
#pd_linear.sort_values(by=pd_linear.index[0], ascending=True, axis=1)
pd_linear.sort_index(axis=0, ascending=True, inplace=True)
pd_linear.to_csv("traces_linear.csv",sep=';')
# Plot
#pd_linear.plot(kind='line', xticks=[2,4,8,16,32,64,128,256,512,1024,2048,4096],
#               xlabel="Total number of connected clients",
#               ylabel="Packet delivery time in seconds")

# Generate for tree
for file in os.listdir():
  filename = os.fsdecode(file)
  match = re.match("trace_tree_([0-9]*)clients", filename)
  if match is not None:
    print("tree:", match.groups()[0])
    result = extract_times(filename)
    fill_result_dict(result)
    #result_values = [i[1] for i in sorted(result.items())]
    #result_values.extend([''] * (8192+2 - len(result.values())))
    #print(result_values)
    #pd_tree.insert(len(pd_tree.columns), int(match.groups()[0]), result.items(),
    #                 allow_duplicates=False)
    pd_append = pandas.DataFrame(index=result.keys(), data=result.values(),
                                 columns=[int(match.groups()[0])])
    pd_tree[int(match.groups()[0])] = pd_append
#pd_tree.sort_values(by=pd_tree.index[0], ascending=True, axis=1)
pd_tree.sort_index(axis=0, ascending=True, inplace=True)
pd_tree.to_csv("traces_tree.csv",sep=';')
# Plot
#pd_tree.plot(kind='line', xticks=[2,4,8,16,32,64,128,256,512,1024,2048,4096],
#               xlabel="Total number of connected clients",
#               ylabel="Packet delivery time in seconds")