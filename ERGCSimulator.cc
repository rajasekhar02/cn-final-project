#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h" //may not be needed here...
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/ergc-application-helper.h"
#include "ns3/log.h"
#include "ns3/callback.h"

#include "ns3/ergc.h"
#include "ns3/ergc-headers.h"
// #include "vector.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ERGCSimulator");

SceneParams sceneparams;
NodeContainer nodesCon;
NodeContainer baseStationCon;
NetDeviceContainer devices;
Ptr<ListPositionAllocator> position;
MobilityHelper nodeMobility;
MobilityHelper baseMobility;
PacketSocketHelper socketHelper;
AquaSimChannelHelper channel;
AquaSimHelper asHelper;
PacketSocketAddress socket;

void initNodes()
{
  nodesCon.Create(sceneparams.no_of_nodes);
  socketHelper.Install(nodesCon);

  std::cout << "Adding Devices to the net device container\n";
  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
  {
    Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
    Ptr<ERGCNodeProps> ergcNodeProps = CreateObject<ERGCNodeProps>();
    ergcNodeProps->nodeType = "UWS";
    ergcNodeProps->BSPosition = baseStationCon.Get(0)->GetObject<MobilityModel>()->GetPosition();
    (*i)->AggregateObject(ergcNodeProps);
    devices.Add(asHelper.Create(*i, newDevice));
  }

  std::cout << "Initializing Nodes Mobility Model" << std::endl;
  nodeMobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", StringValue("ns3::UniformRandomVariable[Min=1|Max=500]"),
                                    "Y", StringValue("ns3::UniformRandomVariable[Min=1|Max=500]"), "Z", StringValue("ns3::UniformRandomVariable[Min=1|Max=500]"));
  nodeMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  nodeMobility.Install(nodesCon);
}

void initBaseStation()
{
  baseStationCon.Create(1);
  socketHelper.Install(baseStationCon);
  Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
  position->Add(ns3::Vector(sceneparams.base_station_x,sceneparams.base_station_y, sceneparams.base_station_z));
  Ptr<ERGCNodeProps> ergcNodeProps = CreateObject<ERGCNodeProps>();
  ergcNodeProps->nodeType = "BS";
  ergcNodeProps->k_mtrs = sceneparams.k_mtrs;
  baseStationCon.Get(0)->AggregateObject(ergcNodeProps);
  devices.Add(asHelper.Create(baseStationCon.Get(0), newDevice));
  newDevice->GetPhy()->SetTransRange(sceneparams.node_communication_range_mtrs);
}

void initBaseMobility()
{
  std::cout << "Initializing BaseStation and Sink Mobility Model" << std::endl;
  baseMobility.SetPositionAllocator(position);
  baseMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  baseMobility.Install(baseStationCon);
}
int main(int argc, char *argv[])
{
  LogComponentEnable("ERGCSimulator", LOG_LEVEL_INFO);
  CommandLine cmd;
  cmd.Parse(argc, argv);
  // aquasim helper
  channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetMac("ns3::AquaSimBroadcastMac");
  asHelper.SetRouting("ns3::ERGCRouting");
  asHelper.SetEnergyModel("ns3::AquaSimEnergyModel", "InitialEnergy", DoubleValue(2.0));
  position = CreateObject<ListPositionAllocator>();
  initBaseStation();
  initBaseMobility();
  initNodes();

  ERGCAppHelper app("ns3::PacketSocketFactory");
  app.SetAttribute("DataRate", DataRateValue(PacketParams::DATA_TRANSMISSION_RATE_BITS_PER_SEC));
  app.SetAttribute("PacketSize", UintegerValue(PacketParams::PACKET_SIZE_BITS));

  ApplicationContainer apps = app.Install(nodesCon);
  apps.Start(Seconds(0.5));
  apps.Stop(Seconds(sceneparams.simulation_rounds));

  ApplicationContainer baseApps = app.Install(baseStationCon);
  baseApps.Start(Seconds(1));
  baseApps.Stop(Seconds(sceneparams.simulation_rounds));

  Simulator::Stop(Seconds(sceneparams.simulation_rounds));
  Simulator::Run();
  Simulator::Destroy();
  std::cout << "finish\n";
  return 0;
}
