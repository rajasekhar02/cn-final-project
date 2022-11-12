#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h" //may not be needed here...
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"

#include "ns3/ERGC.h"
// #include "vector.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ERGCSimulator");

void dummy()
{
  SceneParams sceneparams;

  NodeContainer nodesCon;
  NodeContainer sinksCon;
  NodeContainer senderCon;

  nodesCon.Create(sceneparams.no_of_nodes);
  sinksCon.Create(sceneparams.no_of_sinks);
  senderCon.Create(1);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);
  socketHelper.Install(senderCon);

  // establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  // AquaSimEnergyHelper energy;	//******this could instead be handled by node helper. ****/
  asHelper.SetChannel(channel.Create());
  asHelper.SetMac("ns3::AquaSimBroadcastMac");
  asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(100), "TargetPos", Vector3DValue(Vector(190, 190, 0)));

  /*
   * Preset up mobility model for nodes and sinks here
   */
  MobilityHelper mobility;
  MobilityHelper nodeMobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();

  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
  {
    Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
    devices.Add(asHelper.Create(*i, newDevice));
    newDevice->GetPhy()->SetTransRange(sceneparams.node_communication_range_mtrs);
  }

  // for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
  // {
  //   Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
  //   position->Add(Vector(190, 190, 0));
  //   devices.Add(asHelper.Create(*i, newDevice));
  //   newDevice->GetPhy()->SetTransRange(sceneparams.node_communication_range_mtrs);
  // }
  // std::cout << scenesimStop << " " << nodes << " " << sinks << std::endl;
}

int main(int argc, char *argv[])
{
  LogComponentEnable("ERGCSimulator", LOG_LEVEL_INFO);

  SceneParams sceneparams;
  NodeContainer nodesCon;
  NetDeviceContainer devices;
  MobilityHelper nodeMobility;
  nodesCon.Create(sceneparams.no_of_nodes);
  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);

  // channel
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");

  // aquasim helper
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetMac("ns3::AquaSimBroadcastMac");
  std::cout << "Creating Nodes\n";
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
  {
    Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
    devices.Add(asHelper.Create(*i, newDevice));
  }
  nodeMobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=500.0]"),
                                    "Y", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=500.0]"), "Z", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=500.0]"));
  nodeMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  nodeMobility.Install(nodesCon);
  // for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
  // {

  // }
  Simulator::Stop(Seconds(sceneparams.simulation_rounds));
  Simulator::Run();
  Simulator::Destroy();
  std::cout << "finish\n";
  return 0;
}
