/*
 * Developed by Mehdi Mohammadi
 * Western Michigan University
 * Computer Science Department 
 * July 15, 2014
 */

struct anchor {
  struct anchor *next;
  int node;
  int anchor;
};

struct broadcast_message {
  int nodeid; // node id of the current mote
  unsigned long sent_time; // time when this packet is sent to an anchor mote
  int anchorid; // node id of the anchor mote
  unsigned long recv_time; // time when this packet recieves to the anchor mote
};
