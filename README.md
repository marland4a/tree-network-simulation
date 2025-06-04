Description
==========
This repository holds the code used for the simulation of a tree network vs linear communication.
The [CNET network simulator](https://teaching.csse.uwa.edu.au/units/CITS3002/cnet/) is used to execute the simulation.\
\
The actual code of the simulated nodes is located in network_simulation.c while network_simulation.cnet is the compiled binary.\
Documentation / a flow chart is available in cnet-simulation.drawio.\
\
The results of the simulation were published and presented at the [Vehicular Networking Conference 2025 (VNC2025)](https://vnc2025.ieee-vnc.org/) that took place from 2nd to 4th of June 2025 in Porto, Portugal.
The presented poster is also included in this repository.


Usage
=====
To run and evaluate the simulation, a number of scripts are provided. They can be executed in the following sequence.

1. run topologies/generate_benchmark_topology.py to automatically generate all topology files for the simulation.
2. run the simulation by executing one of the run_simulation_\*.sh scripts.\
   The run_simulation_demo.sh script runs the simulation only for one specific topology file and can be used to verify the topology file generator.\
   The run_simulation_benchmark.sh script runs the simulation for each topologies/benchmark_\*.topology file. The simulation is run once for the tree network and once for the linear network.
3. At this state, the traces of the simulation have been placed in traces\*/traces_linear_\*clients and traces\*/traces_tree_\*clients.\
   To parse the traces into a table, execute the traces\*/parse_traces.py script. It generates the files traces_linear.csv and traces_tree.csv from all trace files in the same directory.
