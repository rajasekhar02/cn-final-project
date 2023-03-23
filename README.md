# Computer Networks Final Project

In this project I have take a research paper which proposes a Energy efficient Grid Routing based on 3d Cubes(EGRC) in Underwater Acoustic Sensor Networks(UASN). I used NS-3 and Aquasim to simulate this protocol.


## Project Layout:
I have divided the Project into 4 branches:

aquasim: This branch contains the code related to aquasim module. I have fixed compiler issues when using this module with NS-3.35.

contrib: This branch contains the code related to contrib folder in the NS-3.35. Here I have implement the ERGC protocol as module.

scratch: this branch contains the code that triggers the simulation.



## Implementation

I have done the following tasks:
- [x] fix the compiler issues for aquasim module
- [x] Implement sending the cube length to all the nodes 
- [x] Based on the cube length determine the Small Cube (cluster) Id
- [x] Elect a leader for the Cluster
- [x] Manage routing for the Cluster head and nodes in the cluster
- [x] Write the Driver Code for Simulations


Chanllenges Faced:
- I spent aleast 30 hours on this project along with other courses.
- I took inspiration from the other source code files such as PacketSocket, Packet Routing, 
