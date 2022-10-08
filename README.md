# CN_final_project

# Terminologies
TWSN = Terrestrial Wireless Sensor Networks

SC  = Small Cubes

WSN = Wireless Sensor Network Nodes

BS = Base Station node

BC = Big Cube

---

k = SC's edge length relates to the communication range of node c

Calculation related to Nodes position and their SC identifier

G(m,n,h) = SC identifier

i(x,y,z) = node i coordinates

m = (k - (x mod k) + x) / k

n = (k - (x mod k) + x) / k

h = (k - (|z| mode k) + |z|) / k

---


D(i,bs) = Distance between node and base station by its own coordinates

D(i,j) = sqrt( pow(x<sub>i</sub> - x<sub>j</sub>, 2) + pow(y<sub>i</sub> - y<sub>j</sub>, 2) + pow(z<sub>i</sub> - z<sub>j</sub>, 2) )

---

E<sub>tx</sub> = Energy Consumption for a node to transmit bit data packets to another node

Below are the equation to calculate based on 

d = distance between the transmitter and receiver

d<sub>0</sub> = threshold distance to transmit data packets

if d < d<sub>0<sub> :
    l * ε<sub>elec</sub> + l * ε<sub>fs</sub>d<sup>2</sup>
else :
    l * ε<sub>elec</sub> + l * ε<sub>mp</sub>d<sup>4</sup>

ε<sub>fs</sub> = transmit amplifier  coefficient of free space

ε<sub>mp</sub> = transmit amplifier  coefficient of multipath

---

---

a(f)^d

a(f) = absorption Coefficient

f = frequency of acoustic signal

d = distance between transmitter and receiver 

---



