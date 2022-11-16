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
NodeContainer sinkCon;
std::vector<ns3::Ptr<ns3::AquaSimNetDevice>> devices;
Ptr<ListPositionAllocator> position;
MobilityHelper nodeMobility;
MobilityHelper baseAndSinkMobility;
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
    double nodeADisFromBase = NodeOperations::distanceBTW(a, NodeOperations::getBaseStationPosition(sceneparams));
    double nodeBDisFromBase = NodeOperations::distanceBTW(b, NodeOperations::getBaseStationPosition(sceneparams));
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
    SCs.push_back(std::make_pair(NodeOperations::SCIndex(nodePosition, sceneparams.k_mtrs), nodePosition));
  }

  auto comparator = [](const std::pair<ns3::Vector, ns3::Vector> &a, const std::pair<ns3::Vector, ns3::Vector> &b)
  {
    double nodeADisFromBase = NodeOperations::distanceBTW(a.second, NodeOperations::getBaseStationPosition(sceneparams));
    double nodeBDisFromBase = NodeOperations::distanceBTW(b.second, NodeOperations::getBaseStationPosition(sceneparams));
    return nodeADisFromBase == nodeBDisFromBase ? 0 : (nodeADisFromBase < nodeBDisFromBase);
  };

  std::sort(SCs.begin(), SCs.end(), comparator);

  std::cout << SCs.size() << std::endl;

  for (int i = 0; i < (int)SCs.size(); i++)
  {
    std::cout << SCs[i].first << " " << SCs[i].second << " " << NodeOperations::distanceBTW(SCs[i].second, NodeOperations::getBaseStationPosition(sceneparams)) << std::endl;
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
    clhsHeader.SetNodeId(AquaSimAddress::ConvertFrom(devices[i]->GetAddress()));
    clhsHeader.SetNodePosition(nodePosition);
    clhsHeader.SetSCIndex(NodeOperations::SCIndex(nodePosition, sceneparams.k_mtrs));
    clhsHeader.SetDistBtwNodeAndBS(NodeOperations::distanceBTW(nodePosition, NodeOperations::getBaseStationPosition(sceneparams)));
    Ptr<AquaSimEnergyModel> em = devices[i]->EnergyModel();
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
    devices.push_back(asHelper.Create(*i, newDevice));
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
  position->Add(NodeOperations::getBaseStationPosition(sceneparams));
  devices.push_back(asHelper.Create(baseStationCon.Get(0), newDevice));
  newDevice->GetPhy()->SetTransRange(sceneparams.node_communication_range_mtrs);
}

void initSinkNodes()
{
  sinkCon.Create(sceneparams.no_of_sinks);
  socketHelper.Install(sinkCon);

  for (NodeContainer::Iterator i = sinkCon.Begin(); i != sinkCon.End(); i++)
  {
    Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
    position->Add(Vector(250, 250, 250));
    devices.push_back(asHelper.Create(*i, newDevice));
    newDevice->GetPhy()->SetTransRange(sceneparams.node_communication_range_mtrs);
  }

  Ptr<Node> sinkNode = sinkCon.Get(0);
  TypeId psfid = TypeId::LookupByName("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket(sinkNode, psfid);
  sinkSocket->Bind(socket);
}

void initBaseAndSinkMobility()
{
  std::cout << "Initializing BaseStation and Sink Mobility Model" << std::endl;
  baseAndSinkMobility.SetPositionAllocator(position);
  baseAndSinkMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  baseAndSinkMobility.Install(sinkCon);
  baseAndSinkMobility.Install(baseStationCon);
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
  asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(500), "TargetPos", Vector3DValue(Vector(0, 0, 0)));
  position = CreateObject<ListPositionAllocator>();
  initNodes();
  initBaseStation();
  initSinkNodes();
  initBaseAndSinkMobility();
  printClusterHeadSelectionHeader();
  // socket.SetAllDevices();
  // socket.SetPhysicalAddress(devices[sceneparams.no_of_nodes]->GetAddress());
  // socket.SetProtocol(0);

  // OnOffHelper app("ns3::PacketSocketFactory", Address(socket));
  // app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0066]"));
  // app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.9934]"));
  // app.SetAttribute("DataRate", DataRateValue(PacketParams::DATA_TRANSMISSION_RATE_BITS_PER_SEC));
  // app.SetAttribute("PacketSize", UintegerValue(PacketParams::PACKET_SIZE_BITS));

  // ApplicationContainer apps = app.Install(nodesCon);
  // apps.Start(Seconds(0.5));
  // apps.Stop(Seconds(sceneparams.simulation_rounds));
  // Packet::EnablePrinting();
  // Ptr<AquaSimEnergyModel> em = devices[0]->EnergyModel();
  // std::cout << "Energy: " << em->GetEnergy() << std::endl;
  Simulator::Stop(Seconds(sceneparams.simulation_rounds));
  Simulator::Run();
  // asHelper.GetChannel()->PrintCounters();
  // std::cout << "Energy: " << em->GetEnergy() << std::endl;
  Simulator::Destroy();
  std::cout << "finish\n";
  return 0;
}
