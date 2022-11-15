/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/vector.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include <cmath>

#ifndef ERGC_H
#define ERGC_H

namespace ns3
{
	class NodeSceneParams
	{
	public:
		static const long int ENERGY_INIT_JOULES = 2;
		static const long int RADIO_DISSP_JOULES_PER_BIT = 50 * 1e-9;
		static constexpr float ABSORPTION_COEFF = 1.001f;
		static const int DELAY_THRESHOLD_SECS = 10;
		static const int DISTANCE_THRESHOLD_MTRS = 87;
		static const long int ENRGY_AGGR_DATA_JOULES_PER_BIT_PER_PKT = 5 * 1e-9;
		static const int SOUND_VELOCITY_MTRS_PER_SEC = 1500;
	};

	class PacketParams
	{
	public:
		static const int PACKET_SIZE_BITS = 500;
		static const int DATA_TRANSMISSION_RATE_BITS_PER_SEC = 50000;
	};

	class SceneParams
	{
	public:
		int big_cube_x_mtrs = 500;
		int big_cube_y_mtrs = 500;
		int big_cube_z_mtrs = 500;
		int base_station_x = 0;
		int base_station_y = 0;
		int base_station_z = 0;
		int simulation_rounds = 100;
		int no_of_sinks = 1;
		int no_of_nodes = 400;
		int k_mtrs = 50;
		int node_velocity = 1;
		int node_communication_range_mtrs = 50;
	};

	// class EnergyModel;
	// class DelayModel;
	// class NodeOperations;
	class EnergyModel
	{
	public:
		// energy for transmission
		static double getEnergyUsedForTransmission(int noOfBitsInAPacket, ns3::Vector sourcePosition, ns3::Vector destinationPosition);
		// energy for receiving
		static double getEnergyUsedForReceiving(int noOfBitsInAPacket);
		// energy for aggregation
		static double getEnergyUsedForAggregation(int noOfBits, int noOfPackets);
	};

	class DelayModel
	{
	public:
		// delay in tranmission and reception time
		static ns3::Time delayForTransmissionAndReceptionTime(ns3::Ptr<ns3::Packet> pkt);
		// delay in propagation time
		static ns3::Time delayDuePropagation(ns3::Vector node1Position, ns3::Vector node2Position);
		// TODO:: delay in byte alignment
		// ----------------------------------
		// total delay
		static ns3::Time totalDelay(ns3::Vector node1Position, ns3::Vector node2Position, ns3::Ptr<ns3::Packet> pkt);
	};

	class NodeOperations
	{
	public:
		static ns3::Vector getBaseStationPosition(SceneParams sp);
		static double distanceBTW(ns3::Vector node1Position, ns3::Vector node2Position);
		static ns3::Vector SCIndex(ns3::Vector nodePosition, int edgeLengthK);
		static ns3::Vector SCIndex2(ns3::Vector nodePosition, int edgeLengthK);
	};
}

#endif /* ERGC_H */
