/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/ergc.h"

namespace ns3
{

  /* ... */
  double EnergyModel::getEnergyUsedForTransmission(int noOfBitsInAPacket, ns3::Vector sourcePosition, ns3::Vector destinationPosition)
  {
    double distBtwSrcAndDest = NodeOperations::distanceBTW(sourcePosition, destinationPosition);
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
    return ns3::Time(NodeOperations::distanceBTW(node1Position, node2Position) / NodeSceneParams::SOUND_VELOCITY_MTRS_PER_SEC);
  }

  ns3::Time DelayModel::totalDelay(ns3::Vector node1Position, ns3::Vector node2Position, ns3::Ptr<ns3::Packet> pkt)
  {
    return ns3::Time(delayForTransmissionAndReceptionTime(pkt) + delayDuePropagation(node1Position, node2Position));
  }

  double NodeOperations::distanceBTW(ns3::Vector node1Position, ns3::Vector node2Position)
  {
    double distX = (node1Position.x - node2Position.x) * (node1Position.x - node2Position.x);
    double distY = (node1Position.y - node2Position.y) * (node1Position.y - node2Position.y);
    double distZ = (node1Position.z - node2Position.z) * (node1Position.z - node2Position.z);
    return std::sqrt(distX + distY + distZ);
  }

  ns3::Vector NodeOperations::SCIndex(ns3::Vector nodePosition, int edgeLengthK)
  {
    int xValue = std::ceil(nodePosition.x);
    int yValue = std::ceil(nodePosition.y);
    int zValue = std::abs(std::ceil(nodePosition.z));
    int m = (edgeLengthK - xValue % edgeLengthK + xValue) / edgeLengthK;
    int n = (edgeLengthK - yValue % edgeLengthK + yValue) / edgeLengthK;
    int h = (edgeLengthK - zValue % edgeLengthK + zValue) / edgeLengthK;
    return Vector(m, n, h);
  }

  ns3::Vector NodeOperations::SCIndex2(ns3::Vector nodePosition, int edgeLengthK)
  {
    int xValue = std::ceil(nodePosition.x);
    int yValue = std::ceil(nodePosition.y);
    int zValue = std::abs(std::ceil(nodePosition.z));
    int m = xValue - (xValue % edgeLengthK);
    int n = yValue - (yValue % edgeLengthK);
    int h = zValue - (zValue % edgeLengthK);
    return Vector(m, n, h);
  }

  ns3::Vector NodeOperations::getBaseStationPosition(SceneParams sp)
  {
    return Vector(sp.base_station_x, sp.base_station_y, sp.base_station_z);
  }
}
