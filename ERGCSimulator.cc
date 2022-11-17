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

void printNodesPosSortByDistBtwNodeAndBS()
{
  uint32_t nNodes = nodesCon.GetN();
  std::vector<ns3::Vector> nodePositions;

  for (uint32_t i = 0; i < nNodes; i++)
  {
    ns3::Vector nodePosition = nodesCon.Get(i)->GetObject<MobilityModel>()->GetPosition();
    nodePositions.push_back(nodePosition);
  }

  auto comparator = [](const ns3::Vector &a, const ns3::Vector &b)
  {
    double nodeADisFromBase = ERGCNodeProps::distanceBTW(a, ERGCNodeProps::getBaseStationPosition(sceneparams));
    double nodeBDisFromBase = ERGCNodeProps::distanceBTW(b, ERGCNodeProps::getBaseStationPosition(sceneparams));
    return nodeADisFromBase == nodeBDisFromBase ? 0 : (nodeADisFromBase < nodeBDisFromBase);
  };

  std::sort(nodePositions.begin(), nodePositions.end(), comparator);

  for (int i = 0; i < (int)nodePositions.size(); i++)
  {
    std::cout << nodePositions[i].x << "," << nodePositions[i].y << "," << nodePositions[i].z << std::endl;
  }
}

void printNodesSCIndices()
{
  uint32_t nNodes = nodesCon.GetN();
  std::vector<std::pair<ns3::Vector, ns3::Vector>> SCs;

  for (uint32_t i = 0; i < nNodes; i++)
  {
    ns3::Vector nodePosition = nodesCon.Get(i)->GetObject<MobilityModel>()->GetPosition();
    SCs.push_back(std::make_pair(ERGCNodeProps::SCIndex(nodePosition, sceneparams.k_mtrs), nodePosition));
  }

  auto comparator = [](const std::pair<ns3::Vector, ns3::Vector> &a, const std::pair<ns3::Vector, ns3::Vector> &b)
  {
    double nodeADisFromBase = ERGCNodeProps::distanceBTW(a.second, ERGCNodeProps::getBaseStationPosition(sceneparams));
    double nodeBDisFromBase = ERGCNodeProps::distanceBTW(b.second, ERGCNodeProps::getBaseStationPosition(sceneparams));
    return nodeADisFromBase == nodeBDisFromBase ? 0 : (nodeADisFromBase < nodeBDisFromBase);
  };

  std::sort(SCs.begin(), SCs.end(), comparator);

  std::cout << SCs.size() << std::endl;

  for (int i = 0; i < (int)SCs.size(); i++)
  {
    std::cout << SCs[i].first << " " << SCs[i].second << " " << ERGCNodeProps::distanceBTW(SCs[i].second, ERGCNodeProps::getBaseStationPosition(sceneparams)) << std::endl;
  }
}

void printClusterHeadSelectionHeader()
{
  // uint32_t nNodes = nodesCon.GetN();
  ClusterHeadSelectionHeader clhsHeader, deserializedHeader;
  Buffer m_buffer;
  for (uint32_t i = 0; i < 1; i++)
  {
    Vector nodePosition = nodesCon.Get(i)->GetObject<MobilityModel>()->GetPosition();
    Ptr<AquaSimNetDevice> aquaNetDevice = devices.Get(0)->GetObject<AquaSimNetDevice>();
    clhsHeader.SetNodeId(AquaSimAddress::ConvertFrom(aquaNetDevice->GetAddress()));
    clhsHeader.SetNodePosition(nodePosition);
    clhsHeader.SetSCIndex(ERGCNodeProps::SCIndex(nodePosition, sceneparams.k_mtrs));
    clhsHeader.SetDistBtwNodeAndBS(ERGCNodeProps::distanceBTW(nodePosition, ERGCNodeProps::getBaseStationPosition(sceneparams)));
    Ptr<AquaSimEnergyModel> em = aquaNetDevice->EnergyModel();
    clhsHeader.SetResidualEnrg(em->GetEnergy());
    std::cout << clhsHeader << std::endl;
    m_buffer.AddAtStart(clhsHeader.GetSerializedSize());
    clhsHeader.Serialize(m_buffer.Begin());
    deserializedHeader.Deserialize(m_buffer.Begin());
    std::cout << deserializedHeader << std::endl;
  }
}

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
    (*i)->AggregateObject(ergcNodeProps);
    devices.Add(asHelper.CreateWithoutRouting(*i, newDevice));
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
  position->Add(ERGCNodeProps::getBaseStationPosition(sceneparams));
  Ptr<ERGCNodeProps> ergcNodeProps = CreateObject<ERGCNodeProps>();
  ergcNodeProps->nodeType = "BS";
  ergcNodeProps->k_mtrs = sceneparams.k_mtrs;
  baseStationCon.Get(0)->AggregateObject(ergcNodeProps);
  devices.Add(asHelper.CreateWithoutRouting(baseStationCon.Get(0), newDevice));
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
  asHelper.SetEnergyModel("ns3::AquaSimEnergyModel", "InitialEnergy", DoubleValue(2.0));
  // asHelper.SetRouting("ms3::Object");
  // asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(500), "TargetPos", Vector3DValue(Vector(0, 0, 0)));
  position = CreateObject<ListPositionAllocator>();
  initNodes();
  initBaseStation();
  initBaseMobility();
  printClusterHeadSelectionHeader();
  // socket.SetAllDevices();
  // socket.SetPhysicalAddress(devices[sceneparams.no_of_nodes]->GetAddress());
  // socket.SetProtocol(0);

  ERGCAppHelper app("ns3::PacketSocketFactory");
  // app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0066]"));
  // app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.9934]"));
  app.SetAttribute("DataRate", DataRateValue(PacketParams::DATA_TRANSMISSION_RATE_BITS_PER_SEC));
  app.SetAttribute("PacketSize", UintegerValue(PacketParams::PACKET_SIZE_BITS));

  ApplicationContainer apps = app.Install(nodesCon);
  apps.Start(Seconds(0.5));
  apps.Stop(Seconds(sceneparams.simulation_rounds));

  ApplicationContainer baseApps = app.Install(baseStationCon);
  baseApps.Start(Seconds(1));
  baseApps.Stop(Seconds(sceneparams.simulation_rounds));

  // Packet::EnablePrinting();

  // Ptr<AquaSimEnergyModel> em = devices.Get(0)->GetObject<AquaSimNetDevice>()->EnergyModel();
  // std::cout << "Energy: " << em->GetEnergy() << std::endl;
  Simulator::Stop(Seconds(sceneparams.simulation_rounds));
  Simulator::Run();
  // asHelper.GetChannel()->PrintCounters();
  // std::cout << "Energy: " << em->GetEnergy() << std::endl;
  Simulator::Destroy();
  std::cout << "finish\n";
  return 0;
}
