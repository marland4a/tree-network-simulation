#!/bin/bash

######################################################
#
# Script to execute the simulation for different numbers
# of clients (i.e. different topology files).
# The simulation is executed for linear and tree networks.
#
######################################################

# Force recompilation because of macros
rm network_simulation.cnet

## Execute for linear networks
# -c : synchronize time_of_day clocks between nodes
# -E : report corrupt frame arival
# -N : provide the number of nodes in the simulation in NNODES
# -O : open each nodes window
# -DNETWORk_LINEAR : simulate linear network
# -T : mirror trace to file
# -W : do not use GUI. Implies -gq
# -e : Execute for the indicated period
for f in ./topologies/benchmark_*.topology
do
  echo "Executing $f ..."
  f_name=$(awk -F'/' '{print $NF}' <<< $f)
  f_numclients=$(awk -F'_' '{print $2}' <<< $f_name | awk -F'.' '{print $1}')
  cnet -c -E -N -O -DNETWORK_LINEAR -T traces/trace_linear_$f_numclients -W -e 3s $f
done


# Force recompilation because of macros
rm network_simulation.cnet

## Execute for tree networks
# -c : synchronize time_of_day clocks between nodes
# -E : report corrupt frame arival
# -N : provide the number of nodes in the simulation in NNODES
# -O : open each nodes window
# -DNETWORk_LINEAR : simulate linear network
# -T : mirror trace to file
# -W : do not use GUI. Implies -gq
# -e : Execute for the indicated period
for f in ./topologies/benchmark_*.topology
do
  echo "Executing $f ..."
  f_name=$(awk -F'/' '{print $NF}' <<< $f)
  f_numclients=$(awk -F'_' '{print $2}' <<< $f_name | awk -F'.' '{print $1}')
  cnet -c -E -N -O -DNETWORK_TREE -T traces/trace_tree_$f_numclients -W -e 3s $f
done
