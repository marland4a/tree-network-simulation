/*
 * Simulate a tree network as well as a classical network architecture
 * with the cnet simulator (https://teaching.csse.uwa.edu.au/units/CITS3002/cnet/index.php)
 * Author: Martin Landsiedel
 * Date: 04/2025
 * 
 * Important:
 *  - Use commandline option -N
 */

#include <cnet.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/* Select between tree and linear network for comparison */
#ifndef NETWORK_TREE
  #ifndef NETWORK_LINEAR
    #error Neither a tree network nor linear network is specified
  #endif
#endif
/* Packet message rate for broadcast */
#ifndef MESSAGERATE
  #warning MESSAGERATE undefined. Using 200000µs for now
  #define MESSAGERATE   200000UL
#endif
#define BROADCAST_TIMER   EV_TIMER1
static const char sample_payload[] = "Lorem ipsum dolor sit amet, consetetur sadipscing elitr,\
  sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.\
  At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata\
  sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr,\
  sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.\
  At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata\
  sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed\
  diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At\
  vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata\
  sanctus est Lorem ipsum dolor sit amet. Duis autem vel eum iriure dolor in hendrerit in vulputate\
  velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan\
  et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla\
  facilisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod\
  tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud\
  exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel\
  eum iriure dolor in hendrerit in vulputate velit es";


/* Print with current time macro */
#define PRINTF_WITH_TIME(format, ...)   printf("[%ld.%06ld] " format, OS->time_of_day.sec, OS->time_of_day.usec, ##__VA_ARGS__)


/* Type of the node: Router or device */
typedef enum {
  NODE_TYPE_ROUTER = 0,             // Node acts as router
  NODE_TYPE_DEVICE,                 // Node acts as device
  NODE_TYPE_SERVER,                 // Node acts as server (thats a special kind of device)
} node_type_t;

/**
 * @brief Node property and state storage
 * Is filled in reboot_node
 */
typedef struct {
  node_type_t node_type;
  CnetAddr node_ip;                 // Node IP address
  CnetAddr node_mac;                // Node MAC address
  CnetAddr tree_addr;               // Node Tree address
} node_state_t;

/* Store state of all nodes */
static node_state_t node_state = {0};


// Space in bytes to reserve in front of the payload to place protocol headers in
#define PACKET_OFFSET_FOR_HEADERS     64U
// Maximum Packet length/size in bytes
#define PACKET_MAX_LENGTH             2500U

/**
 * @brief Packet type to store information of all layers
 */
typedef struct {
  char packet[PACKET_MAX_LENGTH];   // Packet content/data
  size_t packet_start;              // First byte index of content
  size_t packet_end;                // (Last byte + 1) index of content
  bool pass_to_application;         // Whether this packet should be passed to the application.
                                    //  This is used for broadcasts which are sent independently.
  // "IP" protocol
  struct ip {
    CnetAddr destination_addr;
    CnetAddr source_addr;
  } ip;
  // Tree protocol
  struct tree {
    CnetAddr destination_addr;
    CnetAddr source_addr;
  } tree;
  // Tracing
  struct trace {
    struct time_send {              // Time of day when the packet was passed to the physical layer
      long sec;
      long usec;
    } time_send;
    struct time_receive {           // Time of day on reception from the physical layer
      long sec;
      long usec;
    } time_receive;
  } trace;
} packet_t;


/* Prototypes */
// Event Handlers
void reboot_node(CnetEvent ev, CnetTimerID timer, CnetData data);
void physical_ready(CnetEvent ev, CnetTimerID timer, CnetData data);
void application_ready(CnetEvent ev, CnetTimerID timer, CnetData data);
void application_ready_broadcast(CnetEvent ev, CnetTimerID timer, CnetData data);
// Internal
size_t ip_pack(packet_t* packet);
int tree_get_child_offset();


/**
 * @brief Event handler: Node is rebooted
 * This handler is executed once for every node at the start
 * of the simulation.
 * Used to setup and initialize the node.
 */
void reboot_node(CnetEvent ev, CnetTimerID timer, CnetData data)
{
  PRINTF_WITH_TIME("Reboot_node %d event handler\n", OS->nodenumber);
  
  if(NNODES == 0) {
    PRINTF_WITH_TIME("Error: Need to specify -N to cnet to be able to use NNODES");
    exit(1);
  }

  // Initialize: IP = Physical = node number = Tree address
  node_state.node_ip = OS->address;
  node_state.node_mac = OS->address;
  node_state.tree_addr = OS->address;
  // Use only one central router with node number 0
  if(OS->nodenumber == 0) {
    node_state.node_type = NODE_TYPE_ROUTER;
    CHECK(CNET_set_handler(EV_PHYSICALREADY, physical_ready, 0));
  }else {
    node_state.node_type = NODE_TYPE_DEVICE;
    CHECK(CNET_set_handler(EV_PHYSICALREADY, physical_ready, 0));
    CHECK(CNET_set_handler(EV_APPLICATIONREADY, application_ready, 0));
  }

  // Only allow server to send packets
  if(OS->nodenumber == NNODES/2) {
    node_state.node_type = NODE_TYPE_SERVER;
    // Unicast to one node each
    //CHECK(CNET_enable_application(ALLNODES));
    // Broadcast to all nodes
    CHECK(CNET_set_handler(BROADCAST_TIMER, application_ready_broadcast, 0));
    // Alternative: CNET_start_repeating_timer
    //  Non-repeating is used because the packet delivery simulation is deterministic
    //  -> only send one broadcast
    CNET_start_timer(BROADCAST_TIMER, (CnetTime)MESSAGERATE, 0);
  }
}


/**
 * @brief Event Handler: Frame from physical layer received
 * 
 */
void physical_ready(CnetEvent ev, CnetTimerID timer, CnetData data)
{
  packet_t packet;
  size_t packet_size = sizeof(packet_t);
  int in_link;
  CHECK(CNET_read_physical(&in_link, &packet, &packet_size));
  // Fill tracing information
  packet.trace.time_receive.usec = OS->time_of_day.usec;
  packet.trace.time_receive.sec = OS->time_of_day.sec;
  long sec_delta = packet.trace.time_receive.sec - packet.trace.time_send.sec;
  long usec_delta = packet.trace.time_receive.usec - packet.trace.time_send.usec;
  PRINTF_WITH_TIME("Received packet of size %lu from link %d (dt=%ld.%06ld)\n",
                   packet_size, in_link, sec_delta, usec_delta);

  // Node is a router and its application is not the packet destination
  if(node_state.node_type == NODE_TYPE_ROUTER && packet.ip.destination_addr != node_state.node_ip) {
    // For central router, ip = physical address = link
    // -> Forward to link#=ip
    int out_link = packet.ip.destination_addr;
    size_t packet_size = sizeof(packet_t);
    CHECK(CNET_write_physical_reliable(out_link, &packet, &packet_size));
    PRINTF_WITH_TIME("\tRouter: Send packet (dest IP %d) via link %d\n", packet.ip.destination_addr, out_link);
  }
  // Node is a normal device or router received application packet
  else {
    // Check that this node is the packet destination
    if(packet.ip.destination_addr != node_state.node_ip) {
      PRINTF_WITH_TIME("\tDevice: Received IP packet not destined to it. Dropping\n");
      return;
    }
    // Process packet
    #ifdef NETWORK_TREE
      // Packet is destined to this node (or broadcast): Deliver to application
      if(packet.tree.destination_addr == node_state.tree_addr || packet.tree.destination_addr == ALLNODES) {
        size_t packet_size = packet.packet_end - packet.packet_start;
        if(packet.pass_to_application) {
          CHECK(CNET_write_application(packet.packet + PACKET_OFFSET_FOR_HEADERS,
                                        &packet_size));
          PRINTF_WITH_TIME("\tDevice: Tree: Passed packet to application\n");
        }
        else {
          PRINTF_WITH_TIME("\tDevice: Tree: Received packet, not passed to application\n");
        }
        // Print tracing information
        CNET_trace("Node %d received packet from %d after %ld.%08ld secs\n",
                  OS->nodenumber, packet.ip.source_addr, sec_delta, usec_delta);
      }
      // Packet is destined to other or multiple nodes that are reachable
      // by this node: Forward
      // Currently only unicast and broadcast supported
      if(packet.tree.destination_addr != node_state.tree_addr || packet.tree.destination_addr == ALLNODES) {
        if(packet.tree.destination_addr != ALLNODES) {
          printf("Unicast via Tree not implemented. Exiting");
          exit(1);
        }
        // Broadcast: Forward to up to two children
        int child_offset = tree_get_child_offset();       // 0 = this node does not have children; no forwarding required
        if(child_offset != 0) {
          int out_link = 1;
          packet.ip.source_addr = node_state.node_ip;
          packet.ip.destination_addr = node_state.tree_addr - child_offset;    // Left child (IP = Tree address)
          CHECK(CNET_write_physical_reliable(out_link, &packet, &packet_size));
          PRINTF_WITH_TIME("\tDevice: Tree: Send packet (dest IP %d) via link %d\n", packet.ip.destination_addr, out_link);
          packet.ip.destination_addr = node_state.tree_addr + child_offset;    // Right child (IP = Tree address)
          CHECK(CNET_write_physical_reliable(out_link, &packet, &packet_size));
          PRINTF_WITH_TIME("\tDevice: Tree: Send packet (dest IP %d) via link %d\n", packet.ip.destination_addr, out_link);
        }
      }
    #elif defined NETWORK_LINEAR
      size_t packet_size = packet.packet_end - packet.packet_start;
      if(packet.pass_to_application) {
        CHECK(CNET_write_application(packet.packet + PACKET_OFFSET_FOR_HEADERS,
                                     &packet_size));
        PRINTF_WITH_TIME("\tDevice: Linear: Passed packet to application\n");
      }
      else {
        PRINTF_WITH_TIME("\tDevice: Linear: Received packet, not passed to application\n");
      }
      // Print tracing information
      CNET_trace("Node %d received packet from %d after %ld.%08ld secs\n",
                OS->nodenumber, packet.ip.source_addr, sec_delta, usec_delta);
    #endif
  }
}


/**
 * @brief Event Handler: Packet from application layer received
 * 
 */
void application_ready(CnetEvent ev, CnetTimerID timer, CnetData data)
{
  packet_t packet = {0};
  packet.pass_to_application = true;
  CnetAddr destaddr;
  size_t packet_size = PACKET_MAX_LENGTH - PACKET_OFFSET_FOR_HEADERS;
  int ret = CNET_read_application(&destaddr,
                                  packet.packet + PACKET_OFFSET_FOR_HEADERS,
                                  &packet_size);
  if(ret != 0) {
    PRINTF_WITH_TIME("Could not get message from application because of %s. Dropping\n", cnet_errstr[cnet_errno]);
    return;
  }
  packet.packet_start = PACKET_OFFSET_FOR_HEADERS;
  packet.packet_end = PACKET_OFFSET_FOR_HEADERS + packet_size;
  PRINTF_WITH_TIME("Received message to %d of size %lu from application\n", destaddr, packet_size);

  int link;                // Link to send packet through (Determines physical receiver)
  #ifdef NETWORK_TREE
    exit(1);
  #elif defined NETWORK_LINEAR
    packet.ip.destination_addr = destaddr;
    packet.ip.source_addr = node_state.node_ip;
    ip_pack(&packet);
    if(node_state.node_type == NODE_TYPE_ROUTER) {
      link = 0U;                // Loopback to central router
    }else {
      link = 1U;                // Link to central router
    }
  #endif
  
  // Fill tracing information
  packet.trace.time_send.usec = OS->time_of_day.usec;
  packet.trace.time_send.sec = OS->time_of_day.sec;
  // Transmit frame via physical
  packet_size = sizeof(packet_t);
  CHECK(CNET_write_physical_reliable(link, &packet, &packet_size));
  PRINTF_WITH_TIME("\tSend packet to IP %d via link %d\n", packet.ip.destination_addr, link);
}

/**
 * @brief Event Handler: Send out broadcast packet
 * This event is not triggered by the application layer but
 * by the BROADCAST_TIMER event
 */
void application_ready_broadcast(CnetEvent ev, CnetTimerID timer, CnetData data)
{
  packet_t packet = {0};
  // Cannot pass packet to application if we did not receive it from it originally.
  // It would lead to an "corrupted packet" exception.
  packet.pass_to_application = false;
  /*size_t packet_size = PACKET_MAX_LENGTH - PACKET_OFFSET_FOR_HEADERS;
  int ret = CNET_read_application(&destaddr,
                                  packet.packet + PACKET_OFFSET_FOR_HEADERS,
                                  &packet_size);
                                  */
  size_t packet_size = sizeof(sample_payload);
  strncpy(packet.packet, sample_payload, PACKET_MAX_LENGTH);
  packet.packet_start = PACKET_OFFSET_FOR_HEADERS;
  packet.packet_end = PACKET_OFFSET_FOR_HEADERS + packet_size;
  PRINTF_WITH_TIME("Sending broadcast message of size %lu\n", packet_size);

  int link;                // Link to send packet through (Determines physical receiver)
  #ifdef NETWORK_TREE
    if(node_state.node_type == NODE_TYPE_ROUTER) {
      link = 0U;                // Loopback to central router
    }else {
      link = 1U;                // Link to central router
    }
    // Set Tree protocol receiver
    packet.tree.source_addr = node_state.tree_addr;
    packet.tree.destination_addr = ALLNODES;              // Broadcast to all nodes
    // Fill tracing information
    packet.trace.time_send.usec = OS->time_of_day.usec;
    packet.trace.time_send.sec = OS->time_of_day.sec;
    // Transmit packet to first two nodes (via the central router)
    packet_size = sizeof(packet_t);
    size_t packet_size_tmp = packet_size;
    packet.ip.source_addr = node_state.node_ip;
    packet.ip.destination_addr = node_state.node_ip - tree_get_child_offset();
    //packet.ip.destination_addr = (node_state.node_ip+1) / 2U;   // Left node: ceil(middle/2)
    ip_pack(&packet);
    CHECK(CNET_write_physical_reliable(link, &packet, &packet_size_tmp));
    packet_size_tmp = packet_size;
    packet.ip.destination_addr = node_state.node_ip + tree_get_child_offset();
    //packet.ip.destination_addr = node_state.node_ip + node_state.node_ip/2U;  // Right node: middle+floor(middle/2)
    ip_pack(&packet);
    CHECK(CNET_write_physical_reliable(link, &packet, &packet_size_tmp));

  #elif defined NETWORK_LINEAR
    if(node_state.node_type == NODE_TYPE_ROUTER) {
      link = 0U;                // Loopback to central router
    }else {
      link = 1U;                // Link to central router
    }
    // Fill tracing information
    packet.trace.time_send.usec = OS->time_of_day.usec;
    packet.trace.time_send.sec = OS->time_of_day.sec;
    // Transmit packet to every available node (including central router and this node)
    packet.ip.source_addr = node_state.node_ip;
    packet_size = sizeof(packet_t);
    size_t packet_size_tmp;
    for(uint32_t i=0; i < NNODES; i++) {
      packet.ip.destination_addr = i;
      ip_pack(&packet);
      packet_size_tmp = packet_size;
      CHECK(CNET_write_physical_reliable(link, &packet, &packet_size_tmp));
    }
  #endif
}


/**
 * @brief Tree: Pack segment
 * 
 * Generates a Tree header and writes it into the provided
 * packet, in front of the payload.
 * 
 * @param packet Packet to add the Tree header to
 * @return The first byte index of the Tree header in the packet
 *          (=packet.packet_start)
 */
size_t tree_pack(packet_t* packet)
{

}

/**
 * @brief Tree: Unpack segment
 * 
 * Parse the Tree header from the provided packet.
 * 
 * @param packet Packet to parse the Tree header from
 * @return The first byte index of the payload in the packet
 *          (=packet.packet_start)
 */
size_t tree_unpack(packet_t* packet)
{

}


/**
 * @brief "IP": Pack datagram
 * 
 * Generate an "IP" header and write it into the provided
 * packet, in front of the payload/previous header.
 * 
 * @param packet Packet to add the "IP" header to
 * @return The first byte index of the "IP" header in the packet
 *          (=packet.packet_start)
 */
size_t ip_pack(packet_t* packet)
{

}

/**
 * @brief "IP": Unpack datagram
 * 
 * Parse the "IP" header from the provided packet.
 * 
 * @param packet Packet to parse the "IP" header from
 * @return The first byte index of the payload in the packet
 */
size_t ip_unpack(packet_t* packet)
{

}


/**
 * @brief Calculate the delta between this nodes Tree
 * address and the Tree address of either child node.
 * 
 * Formula:
 *  id_child = id_parent ± 1/2 * n / (2^level(id_parent))
 *  with n - Maximum node count of the tree
 *  
 *  id mod n / 2^level
 * 
 *  node_state.tree_addr is taken as id_parent
 * 
 * @note This calculation only works for perfect binary trees
 * (i.e. fully filled tree networks with the server exactly
 * in the middle. NNODES needs to be a power of 2).
 * Otherwise, the number of nodes if the network is filled
 * needs to be calculated first.
 */
int tree_get_child_offset() {
  CnetAddr id_parent = node_state.tree_addr;
  int level_parent = 1;
  // Get Parent level: Require zero modulus
  while(fmod(id_parent, NNODES / pow(2, level_parent)) != 0) {
    level_parent++;
    if(level_parent > 128) {
      exit(2);
    }
  }
  // Calculate child_offset
  int offset = (int)((NNODES / pow(2, level_parent)) / 2);
  return offset;
}
