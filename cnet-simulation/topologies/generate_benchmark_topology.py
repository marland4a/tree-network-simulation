#!/usr/bin/env python3

########################################
#
# Script to generate benchmark_Xclients.topology files
#
# Arguments:
#  $1 = First number of clients to generate
#  $2 = Last number of clients to generate
#  $3 = Math operation to get next number of clients (e.g. *2)
#
########################################

import sys


def generate_for_clients_str(num_clients: int) -> str:
  """
  Generate the file content and return it as string
  """
  result = """
// Generated Topology file for the tree network simulation
// with 8 clients, one server and one central router
//
// The network has a star topology with one central router
// to which every node is connected via a separate LAN
// link/segment.
// Alternatively, all nodes may be connected to the same
// LAN segment, resulting in the same, but bandwidth limited,
// network topology.

compile = "network_simulation.c"

messagerate = 1000ms
maxmessagesize = 2KB


// Size of the network provided to the protocol implementation.
// sizeofnetwork = nodeN + 1
//sizeofnetwork = 4
// This is provided by NNODES if -N switch is used


// WAN links
// Each node is connected to the central router via an own
// WAN link. It is named by the node.
// LAN segments may be used instead, but as every connection
// is one-to-one, there should be no difference.
// To simulate a vehicular LAN segment, 100Mbps bandwidth
// and 1ms latency is defined.
wan-bandwidth   = 100Mbps
wan-propagationdelay = 1ms


// Central router
host node0 {
//router node0 {
"""
  for client in range(1, num_clients):
    result += "\twan to node" + str(client) + " {}\n"
  result += "}\n\n// List of devices\n"
  
  for client in range(1, num_clients):
    result += "host node" + str(client) + " {}"
    if client == (num_clients) / 2:
      result += "\t\t// Server"
    result += '\n'

  result += "\n"
  return result

  
### MAIN ###
if len(sys.argv) != 4:
  print("Invalid number of arguments supplied")
  sys.exit(1)
clients_start = int(sys.argv[1])
clients_end = int(sys.argv[2])
clients_operation = sys.argv[3]
#print(str(clients_start) + " -> " + clients_end + " (" + clients_operation + ")")

clients = clients_start
while clients <= clients_end:
  # Note: Existing files are overwritten
  with open("benchmark_"+str(clients)+"clients.topology", 'w') as f:
    print("Generating " + f.name + " for " + str(clients) + " clients")
    f.write(generate_for_clients_str(clients))
  # Unsafe, but uncritical in this case
  exec('clients = clients ' + clients_operation)
