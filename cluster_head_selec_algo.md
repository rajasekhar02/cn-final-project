```c++
void cluster_head_selection(int k_in_mtrs, double residualEnergy, double distBtwNodeiAndBS, Vector nodeiPosition, Vector nodeiSCIndex, int lengthofTimerTi, AquasimAddress nodeId){
    Simulator::Schedule(lengthOfTimeTi, callTimeOutCallback)
    ClusterT = 1;
    while(Ti != 0) {
        ReadCluMsg(j);
        if(nodeiSCIndex == nodejSCIndex){
            Ci = residualEnergy / distBtnNodeiAndBS;
            Cj = residualEnergyOfJ / distBtnNodeiAndBS;
            if( Ci < Cj) {
                // node i can choose the cluster but node j will make the node i as it child
                ClusterT = 0;
                Ti = 0;
            } else {
                // Node i can be the cluster head for node j
                // Node i has property clusterlist and sizeOfClusterList
                // Node i updates it cluster list
                ClusterList[sizeOfClusterList] = j;
                sizeOfClusterList++;
            }
        }
    }
}

void callTimeOutCallBack(int k_in_mtrs, double residualEnergy, double distBtwNodeiAndBS, Vector nodeiPosition, Vector nodeiSCIndex, int lengthofTimerTi, AquasimAddress nodeId){
    Broadcast(packetWithClusterHeadSelectionHeader) // of the given nodeid
    ReadCluMsg(j);
    if(nodeiSCIndex == nodejSCIndex){
        if(ClusterTj == 1){
            ClusterT = 0;
            ClusterHead = j;
        }
    }
}
```
