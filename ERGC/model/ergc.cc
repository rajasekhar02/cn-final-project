/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/log.h"
#include "ns3/ergc.h"

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("ERGC");

  // NS_OBJECT_ENSURE_REGISTERED(ERGC);

  /* ... */
  double EnergyModel::getEnergyUsedForTransmission(int noOfBitsInAPacket, ns3::Vector sourcePosition, ns3::Vector destinationPosition)
  {
    double distBtwSrcAndDest = ERGCNodeProps::distanceBTW(sourcePosition, destinationPosition);
    if (distBtwSrcAndDest < NodeSceneParams::DISTANCE_THRESHOLD_MTRS)
    {
      return noOfBitsInAPacket * (NodeSceneParams::RADIO_DISSP_JOULES_PER_BIT + std::pow(NodeSceneParams::ABSORPTION_COEFF, distBtwSrcAndDest) * distBtwSrcAndDest * distBtwSrcAndDest);
    }
    return noOfBitsInAPacket * (NodeSceneParams::RADIO_DISSP_JOULES_PER_BIT + std::pow(NodeSceneParams::ABSORPTION_COEFF, distBtwSrcAndDest) * std::pow(distBtwSrcAndDest, 4));
  }
  // energy for receiving
  double EnergyModel::getEnergyUsedForReceiving(int noOfBitsInAPacket)
  {
    return noOfBitsInAPacket * NodeSceneParams::RADIO_DISSP_JOULES_PER_BIT;
  }
  // energy for aggregation
  double EnergyModel::getEnergyUsedForAggregation(int noOfBits, int noOfPackets)
  {
    return NodeSceneParams::ENRGY_AGGR_DATA_JOULES_PER_BIT_PER_PKT * noOfPackets * noOfBits;
  }

  ns3::Time DelayModel::delayForTransmissionAndReceptionTime(ns3::Ptr<ns3::Packet> pkt)
  {
    return ns3::Time((double)pkt->GetSize() / PacketParams::DATA_TRANSMISSION_RATE_BITS_PER_SEC);
  }
  ns3::Time DelayModel::delayDuePropagation(ns3::Vector node1Position, ns3::Vector node2Position)
  {
    return ns3::Time(ERGCNodeProps::distanceBTW(node1Position, node2Position) / NodeSceneParams::SOUND_VELOCITY_MTRS_PER_SEC);
  }

  ns3::Time DelayModel::totalDelay(ns3::Vector node1Position, ns3::Vector node2Position, ns3::Ptr<ns3::Packet> pkt)
  {
    return ns3::Time(delayForTransmissionAndReceptionTime(pkt) + delayDuePropagation(node1Position, node2Position));
  }

  ERGCNodeProps::ERGCNodeProps()
  {
    m_nodeType = "";
    m_scIndex = ns3::Vector(0, 0, 0);
    m_k_mtrs = 0;
  }
  TypeId
  ERGCNodeProps::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::ERGCNodeProps")
                            .SetParent<ns3::Object>()
                            .AddConstructor<ERGCNodeProps>();
    return tid;
  }
  double
  ERGCNodeProps::distanceBTW(ns3::Vector node1Position, ns3::Vector node2Position)
  {
    double distX = (node1Position.x - node2Position.x) * (node1Position.x - node2Position.x);
    double distY = (node1Position.y - node2Position.y) * (node1Position.y - node2Position.y);
    double distZ = (node1Position.z - node2Position.z) * (node1Position.z - node2Position.z);
    return std::sqrt(distX + distY + distZ);
  }

  double
  ERGCNodeProps::distanceBTWSCToBS(ns3::Vector scIndex, ns3::Vector BSPosition, u_int32_t k_mtrs)
  {
    return k_mtrs * ERGCNodeProps::distanceBTW(scIndex, BSPosition);
  }

  ns3::Vector
  ERGCNodeProps::SCIndex(ns3::Vector nodePosition, int edgeLengthK)
  {
    int xValue = std::floor(nodePosition.x);
    int yValue = std::floor(nodePosition.y);
    int zValue = std::abs(std::ceil(nodePosition.z));
    int m = (edgeLengthK - xValue % edgeLengthK + xValue) / edgeLengthK;
    int n = (edgeLengthK - yValue % edgeLengthK + yValue) / edgeLengthK;
    int h = (edgeLengthK - zValue % edgeLengthK + zValue) / edgeLengthK;
    return Vector(m, n, h);
  }

  ns3::Vector
  ERGCNodeProps::SCIndex2(ns3::Vector nodePosition, int edgeLengthK)
  {
    int xValue = std::ceil(nodePosition.x);
    int yValue = std::ceil(nodePosition.y);
    int zValue = std::abs(std::ceil(nodePosition.z));
    int m = xValue - (xValue % edgeLengthK);
    int n = yValue - (yValue % edgeLengthK);
    int h = zValue - (zValue % edgeLengthK);
    return Vector(m, n, h);
  }

  ns3::Time
  ERGCNodeProps::GetWaitTimeToBroadCastClusterHeadMsg(double NodeResidualEnergy, Vector currentNodePosition, Time Tmax)
  {
    double distBtwSCAndBS = ERGCNodeProps::distanceBTWSCToBS(m_scIndex, m_BSPosition, m_k_mtrs);
    double distBtwNodeAndBS = ERGCNodeProps::distanceBTW(currentNodePosition, m_BSPosition);
    double energyRatio = NodeResidualEnergy / m_netDeviceInitialEnergy;
    double distanceRatio = distBtwSCAndBS - distBtwNodeAndBS / distBtwSCAndBS;
    NS_LOG_DEBUG("Energy Ratio: " << energyRatio << " distanceRatio: " << distanceRatio);
    return Time(Tmax * energyRatio * distanceRatio);
  }

  double ERGCNodeProps::GetSqrt3Dist()
  {
    return m_k_mtrs * sqrt(3);
  }

  double ERGCNodeProps::GetSqrt6Dist()
  {
    return m_k_mtrs * sqrt(6);
  }
}
