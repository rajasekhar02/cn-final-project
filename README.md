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
- [x] Implement packet creation and transmission for the EGRC protocol.
- [x] Write the Driver Code for Simulations
- [x] Write a technical report summarizing the protocol and the results of the simulation study.

## Challenges
* Understanding and implementing the EGRC protocol proposed in the research paper was a challenging task.
* Debugging and resolving errors in the code required a significant amount of time and effort.
* Coordinating and integrating the code changes across different branches and modules was a complex process.
* Keeping up with the project timeline and deadlines was a challenge, especially while juggling other coursework and commitments.
* The lack of prior experience with C++ and NS-3 made the learning curve steep and required additional effort to get up to speed.
* The need to constantly refer to external resources, such as documentation and forums, to solve issues and implement features added to the complexity of the project.

