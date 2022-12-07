#include "ergc-application.h"
#include "ergc-routing.h"
#include "ergc-headers.h"
#include "ergc.h"
#include "ns3/aqua-sim-header.h"
#include "ns3/aqua-sim-address.h"
#include "ns3/packet-socket.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include <cstdio>

namespace ns3
{

	NS_LOG_COMPONENT_DEFINE("ERGCRouting");
	NS_OBJECT_ENSURE_REGISTERED(ERGCRouting);

	ERGCRouting::ERGCRouting() : m_hasSetRouteFile(false), m_hasSetNode(false)
	{
	}

	ERGCRouting::ERGCRouting(char *routeFile) : m_hasSetNode(false)
	{
		SetRouteTable(routeFile);
	}

	ERGCRouting::~ERGCRouting()
	{
	}

	TypeId
	ERGCRouting::GetTypeId()
	{
		static TypeId tid = TypeId("ns3::ERGCRouting")
								.SetParent<AquaSimRouting>()
								.AddConstructor<ERGCRouting>();
		return tid;
	}

	int64_t
	ERGCRouting::AssignStreams(int64_t stream)
	{
		NS_LOG_FUNCTION(this << stream);
		return 0;
	}

	void
	ERGCRouting::SetRouteTable(char *routeFile)
	{
		m_hasSetRouteFile = false;
		strcpy(m_routeFile, routeFile);
		// if( m_hasSetNode ) {
		ReadRouteTable(m_routeFile);
		//}
	}

	/*
	 * Load the static routing table in filename
	 *
	 * @param filename   the file containing routing table
	 * */
	void
	ERGCRouting::ReadRouteTable(char *filename)
	{
		NS_LOG_FUNCTION(this);

		FILE *stream = fopen(filename, "r");
		int current_node, dst_node, nxt_hop;

		if (stream == NULL)
		{
			printf("ERROR: Cannot find routing table file!\nEXIT...\n");
			exit(0);
		}

		while (!feof(stream))
		{
			fscanf(stream, "%d:%d:%d", &current_node, &dst_node, &nxt_hop);

			if (AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() == current_node)
			{
				m_rTable[AquaSimAddress(dst_node)] = AquaSimAddress(nxt_hop);
			}
		}

		fclose(stream);
	}

	bool
	ERGCRouting::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
	{
		NS_LOG_FUNCTION(this << p);
		AquaSimHeader ash;
		// struct hdr_ip* ih = HDR_IP(p);
		p->PeekHeader(ash);

		// NS_LOG_DEBUG("Me(" << AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() << "): Received packet from MAC : " << ash.GetSize() << " bytes ; " << ash.GetTxTime().GetSeconds() << " sec. ; Dest: " << ash.GetDAddr().GetAsInt() << " ; Src: " << ash.GetSAddr().GetAsInt() << " ; Next H.: " << ash.GetNextHop().GetAsInt());
		if (IsDeadLoop(p))
		{
			NS_LOG_INFO("Dropping packet " << p << " due to route loop");
			// drop(p, DROP_RTR_ROUTE_LOOP);
			p = 0;
			return false;
		}
		else if (AmISrc(p))
		{
			p->RemoveHeader(ash);
			ash.SetSize(ash.GetSize() + SR_HDR_LEN); // add the overhead of static routing's header
			p->AddHeader(ash);
		}
		else if (!AmINextHop(p))
		{
			NS_LOG_INFO("Dropping packet " << p << " due to duplicate");
			// drop(p, DROP_MAC_DUPLICATE);
			p = 0;
			return false;
		}

		// increase the number of forwards
		p->RemoveHeader(ash);
		uint8_t numForwards = ash.GetNumForwards() + 1;
		ash.SetNumForwards(numForwards);
		p->AddHeader(ash);

		if (AmIDst(p) || (ash.GetDirection() == AquaSimHeader::UP))
		{
			NS_LOG_INFO("I am destination. Sending up.");
			AquaSimHeader asHeader;
			// p->PeekHeader(asHeader);
			p->RemoveHeader(asHeader);
			AquaSimAddress from = asHeader.GetSAddr();
			AquaSimAddress to = asHeader.GetDAddr();
			// std::cout << (from == m_device->GetAddress()) << (to == AquaSimAddress::GetBroadcast()) << std::endl;
			// // // SendUp(p);
			Ptr<AquaSimNetDevice> aqd = GetNetDevice();
			aqd->Receive(p, 0, AquaSimAddress::ConvertFrom(m_device->GetAddress()), from);
			return true;
		}

		// find the next hop and forward
		AquaSimAddress next_hop = FindNextHop(p);
		if (next_hop != AquaSimAddress::GetBroadcast())
		{
			SendDown(p, next_hop, Seconds(0.0));
			return true;
		}
		else if (ash.GetDirection() == AquaSimHeader::DOWN)
		{
			SendDown(p, next_hop, Seconds(0.0));
			return true;
		}
		return false;
	}

	ClusterNeighborHeader ERGCRouting::getBestClusterNeighbor(double distanceBtwDeviceAndBS,double deviceResidualEnergy, Vector nodePosition, const Ptr<Packet> p){
		double minVertex = std::numeric_limits<double>::max();
		ClusterNeighborHeader minVertextHeader = m_neighbor_cluster_table.begin()->second;
		for(auto eachNeighbor:m_neighbor_cluster_table){
			ClusterNeighborHeader clusterNeighbor = eachNeighbor.second;
			double distanceRatio = clusterNeighbor.GetDistBtwNodeAndBS()/distanceBtwDeviceAndBS;
			double energyRatio = clusterNeighbor.GetResidualEnrg() / deviceResidualEnergy;
			double delayRatio = DelayModel::totalDelay(nodePosition,clusterNeighbor.GetNodePosition(),p).ToDouble(ns3::Time::Unit::S)/NodeSceneParams::DELAY_THRESHOLD_SECS;
			double vertexRatio = distanceRatio + (NodeSceneParams::LAMBDA*(1-energyRatio)) + (NodeSceneParams::MU*delayRatio);
			if(vertexRatio < minVertex){
				minVertex = vertexRatio;
				minVertextHeader = clusterNeighbor;
			}
		}
	 	return minVertextHeader;
	}

	/*
	 * @param p   a packet
	 * @return    the next hop to route packet p
	 * */
	AquaSimAddress
	ERGCRouting::FindNextHop(const Ptr<Packet> p)
	{
		AquaSimHeader ash;
		p->PeekHeader(ash);
		/*
			A normal node knows only the cluster head node & BaseStation
			A cluster head node knows the cluster nodes & cluster neighbor nodes & BaseStation
		*/
		if(ash.GetDAddr() == AquaSimAddress::GetBroadcast()){
			return AquaSimAddress::GetBroadcast();
		}
		Ptr<ERGCApplication> application = GetNetDevice()->GetNode()->GetApplication(0)->GetObject<ERGCApplication>();
		double distanceBtwNodeAndBS = application->getDistBtwNodeAndBS();
		double residualEnergy =  application->getResidualEnergy();
		AquaSimAddress baseStationAddress = application->getBaseStationAddress();
		uint32_t m_k_mtrs = application->getResidualEnergy();
		Vector nodePosition = application->getNodePosition();
	 	// std::map<AquaSimAddress, ClusterNeighborHeader>::iterator it2 = m_neighbor_cluster_table.begin();	
		if(ash.GetDAddr() == baseStationAddress && distanceBtwNodeAndBS < 2*m_k_mtrs){
			return baseStationAddress;
		}
		ClusterNeighborHeader bestClusterNeighbor = getBestClusterNeighbor(distanceBtwNodeAndBS, residualEnergy, nodePosition, p);
		// std::map<AquaSimAddress, AquaSimAddress>::iterator it = m_rTable.find(ash.GetDAddr());
		// return it == m_rTable.end() ? AquaSimAddress::GetBroadcast() : it->second;
		return bestClusterNeighbor.GetClusterHeadId();
	}

} // namespace ns3
