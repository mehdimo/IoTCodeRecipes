Internet of Things Code Recipes
================================

The Internet of Things (IoT) aims to connect everything to the Internet by lightweight protocols and standards. Some commonly used protocols are presented here as an example of how an IoT application works.

There are three separate samples that pertain to different aspects of IoT.
- Service Discovery
- Wireless Sensor Network
- Cloud Computing

To run these programs you need to have Contaki and Cooja installed on your machine. To get started with Contaki and cooja refer to  its [quickstart](http://www.contiki-os.org/start.html).
If you have your contiki folder in /home/user/ you don't need to change the CONTIKI variable in MAKEFILEs; Otherwise, you have to change it to the installed path of contiki.

Service Discovery 
-----------------

This is an implementation and simulation of a service discovery protocol for Wireless Sensor Networks using multicast DNS (mDNS).

Every node that joins the network broadcasts a message containing its service description to the other nodes across the network. 
All client nodes that receive this packet register the service description and its corresponding provider to a local service lookup table. 
When a service is desired, the client seeks the service description in its lookup table. 
When a client finds the address of a service provider in its lookup table, it then sends a unicast message to the service provider requesting the desired service.

To run the program, you can open the prepared project with .csc extension.
You can also follow these steps to configure your own project:

1- Open Cooja simulator, create a new project and set a name for that. You can keep the default settings. 
2- Create a mote type based on Sky Mote.
3- Upload the processing code (service_discovery_IoT.c) to the mote.
4- Compile the code.
5- Create mote type.
6- Now you can add several instances of that mote to your environment.
7- Start the simulation.


Wireless Sensor Network
-----------------------

This project is a simulation of wireless sensor network in which a sink node have to receive packets from motes to build a local table that identifies the closest anchor node to every non-anchor node in the network. 
Three different types of motes have been defined to address this problem:
- Normal Motes that should send their closest anchor to the sink.
- Anchor Motes which are some special motes near normal motes.
- Sink Mote which is designed to gather information that comes from other motes.

We assume the signal speed is unified across the network. Also our measurement packets are in a same structure and size. So, packet propagation delay could be an indication of distance.
ContikiRPL which is an implementation of RPL is used in this model as routing protocol. The underlying network layer is IPv6. 

To run the program, open the prepared project with .csc extensionthe or follow the below steps to create a new project:

1- Open Cooja simulator, create a new project and set a name for it. You can use default settings in your project. 
2- Create a Sky Mote and name it Normal mote.
3- Upload the processing code (normal_mote.c) to the Normal mote.
4- Compile Normal mote.
5- Create mote type.
6- Create a Sky Mote and name it Anchor mote.
7- Upload the processing code (anchor_mote.c) to the Anchor mote.
8- Compile the Anchor mote.
9- Create mote type.
10- Create another Sky Mote and name it Sink mote.
11- Upload the processing code (sink_mote.c) to the Sink mote.
12- Compile the Sink mote
13- Create mote type.
14- Now you can add several instances of these motes to your environment.
15- Start the program.

Cloud Computing
---------------
