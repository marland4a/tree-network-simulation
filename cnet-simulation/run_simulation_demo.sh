#!/bin/bash

######################################################
#
# Convenience script to execute the simulation
#
# Pass a topology file as first argument
#
######################################################

# Force recompilation
rm network_simulation.cnet

## Execute for linear networks
# -c : synchronize time_of_day clocks between nodes
# -E : report corrupt frame arival
# -N : provide the number of nodes in the simulation in NNODES
# -O : open each nodes window
# -DNETWORk_LINEAR : simulate linear network
# -T : mirror trace to file
# -g : Commence execution right after startup
cnet -c -E -N -O -DNETWORK_LINEAR -T traces/trace_linear.txt -g $1 #network_simulation.topology

# Force recompilation
rm network_simulation.cnet

## Execute for tree networks
# -c : synchronize time_of_day clocks between nodes
# -E : report corrupt frame arival
# -N : provide the number of nodes in the simulation in NNODES
# -O : open each nodes window
# -DNETWORK_TREE : simulate tree network
# -T : mirror trace to file
# -g : Commence execution right after startup
cnet -c -E -N -O -DNETWORK_TREE -T traces/trace_tree.txt -g $1 #network_simulation.topology

