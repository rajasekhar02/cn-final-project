
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/aqua-sim-net-device.h"
#include "ergc-application.h"
#include "ergc.h"
#include "ergc-headers.h"
#include "ergc.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("ERGCApplication");

    NS_OBJECT_ENSURE_REGISTERED(ERGCApplication);

    TypeId
    ERGCApplication::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::ERGCApplication")
                                .SetParent<Application>()
                                .SetGroupName("Applications")
                                .AddConstructor<ERGCApplication>()
                                .AddAttribute("DataRate", "The data rate in on state.",
                                              DataRateValue(DataRate("500kb/s")),
                                              MakeDataRateAccessor(&ERGCApplication::m_cbrRate),
                                              MakeDataRateChecker())
                                .AddAttribute("PacketSize", "The size of packets sent in on state",
                                              UintegerValue(512),
                                              MakeUintegerAccessor(&ERGCApplication::m_pktSize),
                                              MakeUintegerChecker<uint32_t>(1))
                                .AddAttribute("Remote", "The address of the destination",
                                              AddressValue(),
                                              MakeAddressAccessor(&ERGCApplication::m_peer),
                                              MakeAddressChecker())
                                .AddAttribute("Local",
                                              "The Address on which to bind the socket. If not set, it is generated automatically.",
                                              AddressValue(),
                                              MakeAddressAccessor(&ERGCApplication::m_local),
                                              MakeAddressChecker())
                                .AddAttribute("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                                              StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                                              MakePointerAccessor(&ERGCApplication::m_onTime),
                                              MakePointerChecker<RandomVariableStream>())
                                .AddAttribute("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                                              StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                                              MakePointerAccessor(&ERGCApplication::m_offTime),
                                              MakePointerChecker<RandomVariableStream>())
                                .AddAttribute("MaxBytes",
                                              "The total number of bytes to send. Once these bytes are sent, "
                                              "no packet is sent again, even in on state. The value zero means "
                                              "that there is no limit.",
                                              UintegerValue(0),
                                              MakeUintegerAccessor(&ERGCApplication::m_maxBytes),
                                              MakeUintegerChecker<uint64_t>())
                                .AddAttribute("Protocol", "The type of protocol to use. This should be "
                                                          "a subclass of ns3::SocketFactory",
                                              TypeIdValue(UdpSocketFactory::GetTypeId()),
                                              MakeTypeIdAccessor(&ERGCApplication::m_tid),
                                              // This should check for SocketFactory as a parent
                                              MakeTypeIdChecker())
                                .AddAttribute("EnableSeqTsSizeHeader",
                                              "Enable use of SeqTsSizeHeader for sequence number and timestamp",
                                              BooleanValue(false),
                                              MakeBooleanAccessor(&ERGCApplication::m_enableSeqTsSizeHeader),
                                              MakeBooleanChecker())
                                .AddTraceSource("Tx", "A new packet is created and is sent",
                                                MakeTraceSourceAccessor(&ERGCApplication::m_txTrace),
                                                "ns3::Packet::TracedCallback")
                                .AddTraceSource("TxWithAddresses", "A new packet is created and is sent",
                                                MakeTraceSourceAccessor(&ERGCApplication::m_txTraceWithAddresses),
                                                "ns3::Packet::TwoAddressTracedCallback")
                                .AddTraceSource("TxWithSeqTsSize", "A new packet is created with SeqTsSizeHeader",
                                                MakeTraceSourceAccessor(&ERGCApplication::m_txTraceWithSeqTsSize),
                                                "ns3::PacketSink::SeqTsSizeCallback");
        return tid;
    }

    ERGCApplication::ERGCApplication()
        : m_connected(false),
          m_residualBits(0),
          m_lastStartTime(Seconds(0)),
          m_totBytes(0),
          m_unsentPacket(0),
          m_socket(0)
    {
        NS_LOG_FUNCTION(this);
    }

    ERGCApplication::~ERGCApplication()
    {
        NS_LOG_FUNCTION(this);
    }

    bool
    ERGCApplication::isClusterHead()
    {
        return m_clusterHead;
    }

    void
    ERGCApplication::SetMaxBytes(uint64_t maxBytes)
    {
        NS_LOG_FUNCTION(this << maxBytes);
        m_maxBytes = maxBytes;
    }

    Ptr<Socket>
    ERGCApplication::GetSocket(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_socket;
    }

    u_int32_t ERGCApplication::getK()
    {
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        return ergcNodeProps->m_k_mtrs;
    }

    double ERGCApplication::getSqrt3Dist()
    {
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        return ergcNodeProps->GetSqrt3Dist();
    }

    double ERGCApplication::getSqrt6Dist()
    {
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        return ergcNodeProps->GetSqrt6Dist();
    }

    ns3::Vector ERGCApplication::getNodePosition()
    {
        return GetNode()->GetObject<MobilityModel>()->GetPosition();
    }

    ns3::Vector ERGCApplication::getBSPosition()
    {
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        return ergcNodeProps->m_BSPosition;
    }

    ns3::Vector ERGCApplication::getSCIndex()
    {
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        return ergcNodeProps->m_scIndex;
    }

    double ERGCApplication::getResidualEnergy()
    {
        Ptr<AquaSimNetDevice> asqd = GetNode()->GetDevice(0)->GetObject<AquaSimNetDevice>();
        return asqd->EnergyModel()->GetEnergy();
    }

    double ERGCApplication::getDistBtwNodeAndBS()
    {
        ns3::Vector nodePosition = GetNode()->GetObject<MobilityModel>()->GetPosition();
        return ERGCNodeProps::distanceBTW(nodePosition, getBSPosition());
    }

    AquaSimAddress ERGCApplication::getNodeId()
    {
        return AquaSimAddress::ConvertFrom(GetNode()->GetDevice(0)->GetAddress());
    }

    void
    ERGCApplication::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);

        // CancelEvents();
        m_socket = 0;
        m_unsentPacket = 0;
        // chain up
        Application::DoDispose();
    }

    // Application Methods
    void ERGCApplication::StartApplication() // Called at time specified by Start
    {
        NS_LOG_FUNCTION(this);
        PacketSocketAddress pSocket;
        pSocket.SetAllDevices();
        pSocket.SetPhysicalAddress((Address)AquaSimAddress::GetBroadcast());
        pSocket.SetProtocol(0);
        this->m_peer = pSocket;
        if (!m_socket)
        {
            m_socket = Socket::CreateSocket(GetNode(), m_tid);
            int ret = -1;
            ret = m_socket->Bind();
            if (ret == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(m_peer);
            m_socket->SetAllowBroadcast(true);
            m_socket->SetConnectCallback(
                MakeCallback(&ERGCApplication::ConnectionSucceeded, this),
                MakeCallback(&ERGCApplication::ConnectionFailed, this));
        }
        m_socket->SetRecvCallback(MakeCallback(&ERGCApplication::HandleRead, this));
        std::string nodeType = GetNode()->GetObject<ERGCNodeProps>()->m_nodeType;
        if (nodeType == "BS")
        {
            m_receivedK = true;
            handleBSStartEvent();
        }
        else
        {
            handleNodeStartEvent();
        }
    }

    void
    ERGCApplication::handleBSStartEvent()
    {
        m_BSSentKEvent = Simulator::Schedule(Simulator::Now(), &ERGCApplication::SendPacketWithK, this);
    }
    void
    ERGCApplication::handleNodeStartEvent()
    {
    }

    void
    ERGCApplication::SendPacketWithK()
    {
        NS_LOG_FUNCTION(this);
        u_int32_t k_mtrs = GetNode()->GetObject<ERGCNodeProps>()->m_k_mtrs;
        CubeLengthHeader cubeLengthHeader;
        cubeLengthHeader.SetKMtrs(k_mtrs);
        Ptr<Packet> packet = Create<Packet>(cubeLengthHeader.GetSerializedSize());
        packet->AddHeader(cubeLengthHeader);
        m_socket->Send(packet);
        m_lastStartTime = Simulator::Now();
        NS_LOG_INFO("BS Broadcasted k: " << k_mtrs << " " << m_lastStartTime << '\n');
    }

    void
    ERGCApplication::HandleRead(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);
        Ptr<Packet> packet;
        Address localAddress;
        while ((packet = socket->Recv()))
        {
            socket->GetSockName(localAddress);

            if (packet->GetSize() <= 0)
                return;

            CubeLengthHeader cubeLengthTs;
            ClusterHeadSelectionHeader clusterHSH;
            // std::cout<<packet->PeekHeader(clusterHSH)<<std::endl;
            //
            if (packet->PeekHeader(cubeLengthTs) && !m_receivedK)
            {
                HandleCubeLengthAssignPacket(packet);
                packet = 0;
                StartClusteringPhase();
                return;
            }

            if (packet->PeekHeader(clusterHSH))
            {
                HandleClusterHeadSelection(packet);
                // if (!m_clusterHead && !m_clusHeadNotifyEvent.IsExpired())
                // {
                //
                //     return;
                // }
                // if (isClusterHead)
                // {
                //     StartQueryingClusterNeighborPhase();
                // }
                // StartSendingData();
                // if (!m_clusHeadNotifyEvent.IsExpired())
                // {
                //     Simulator::Cancel(m_clusHeadNotifyEvent);
                // }
                // m_clusHeadNotifyEvent = Simulator::Schedule(m_maxClusterHeadSelectionTime, &ERGCApplication::NotifyNodesThatIamClusterHead, this);
                return;
            }
            ClusterNeighborHeader clusterNH;
            if (packet->PeekHeader(clusterNH))
            {
                HandleAddClusterNeighbor(packet);
                return;
            }
            return;
        }
    }

    void ERGCApplication::HandleCubeLengthAssignPacket(Ptr<Packet> &packet)
    {
        CubeLengthHeader cubeLengthTs;
        packet->RemoveHeader(cubeLengthTs);
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        ergcNodeProps->m_k_mtrs = cubeLengthTs.GetKMtrs();
        m_receivedK = true;
        NS_LOG_INFO("Node " << GetNode()->GetId() << " Received k: " << cubeLengthTs.GetKMtrs());
    }

    void ERGCApplication::HandleClusterHeadSelection(Ptr<Packet> packet)
    {
        ClusterHeadSelectionHeader clusHeadSelectionH;
        packet->RemoveHeader(clusHeadSelectionH);
        NS_LOG_INFO(getSCIndex() << "---" << clusHeadSelectionH.GetSCIndex());
        if (getSCIndex() == clusHeadSelectionH.GetSCIndex())
        {
            double Ci = getResidualEnergy() / getDistBtwNodeAndBS();
            double Cj = clusHeadSelectionH.GetResidualEnrg() / clusHeadSelectionH.GetDistBtwNodeAndBS();
            NS_LOG_DEBUG("Node Id: " << getNodeId() << " Ci " << Ci << " Node j Id: " << clusHeadSelectionH.GetNodeId() << " Cj " << Cj << " " << m_sendClusMsgEvent.IsRunning());

            // if (m_sendClusMsgEvent.IsRunning())
            // Simulator::Now().Compare(m_broadcastClusHeadStartTime)
            if (m_sendClusMsgEvent.IsRunning())
            {

                NS_LOG_INFO("Listening for cluster initialization msg");
                // double Ci = getResidualEnergy() / getDistBtwNodeAndBS();
                // double Cj = clusHeadSelectionH.GetResidualEnrg() / clusHeadSelectionH.GetDistBtwNodeAndBS();
                // NS_LOG_DEBUG("Node Id: " << getNodeId() << " Ci " << Ci << " Node j Id: " << clusHeadSelectionH.GetNodeId() << " Cj " << Cj);
                if (Ci < Cj)
                {
                    m_clusterHead = false;
                    NS_LOG_DEBUG("Not a cluster Head: " << getNodeId());
                    Simulator::Cancel(m_sendClusMsgEvent);
                }
                else
                {
                    //             // Node i can be the cluster head for node j
                    //             // Node i has property clusterlist and sizeOfClusterList
                    //             // Node i updates it cluster list
                    m_clusterList[clusHeadSelectionH.GetNodeId()] = clusHeadSelectionH;
                    // sizeOfClusterList++;
                }
                return;
            }
            // }
            if (clusHeadSelectionH.GetIsClusterHeadMsg())
            {
                // how will i find the what are the nodes that are cluster heads
                // where will i store the information
                // after the Tmax out every cluster head will send msg to its children indicating that it is cluster head
                m_clusterHead = false;
                m_clusterHeadInfo = clusHeadSelectionH;
                NS_LOG_DEBUG("Cluster Head Node " << clusHeadSelectionH.GetNodeId() << " Cluster Head SC Index" << clusHeadSelectionH.GetSCIndex());
            }
        }
    }

    void ERGCApplication::StartClusteringPhase()
    {
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        ergcNodeProps->m_scIndex = ERGCNodeProps::SCIndex(getNodePosition(), getK());
        NS_LOG_INFO("Node " << GetNode()->GetId() << " SC Index: " << (ergcNodeProps->m_scIndex));
        ScheduleClusterHeadSelectionPhase();
    }

    void ERGCApplication::ScheduleClusterHeadSelectionPhase()
    {
        ns3::Vector nodePosition = GetNode()->GetObject<MobilityModel>()->GetPosition();
        Ptr<AquaSimNetDevice> asqd = GetNode()->GetDevice(0)->GetObject<AquaSimNetDevice>();
        double residualEnergy = asqd->EnergyModel()->GetEnergy();
        Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
        m_broadcastClusHeadTimeOut = ergcNodeProps->GetWaitTimeToBroadCastClusterHeadMsg(
            residualEnergy,
            nodePosition,
            m_maxClusterHeadSelectionTime);
        if (m_broadcastClusHeadTimeOut.GetDouble() < 0)
        {
            m_clusterHead = false;
            return;
        }
        NS_LOG_INFO("Timeout time: " << (m_broadcastClusHeadTimeOut).GetDouble());
        Ptr<AquaSimNetDevice> device = GetNode()->GetDevice(0)->GetObject<AquaSimNetDevice>();
        device->GetPhy()->SetTransRange(getSqrt3Dist());
        NS_LOG_DEBUG("Node Id: " << getNodeId() << " SC Index " << getSCIndex() << " At: " << Simulator::Now() << " Timeout Time: " << m_broadcastClusHeadTimeOut);
        m_broadcastClusHeadStartTime = Simulator::Now();
        m_sendClusMsgEvent = Simulator::Schedule(m_broadcastClusHeadTimeOut, &ERGCApplication::clusterHeadMessageTimeout, this);
        Simulator::Schedule(m_broadcastClusHeadTimeOut + m_maxClusterHeadSelectionTime, &ERGCApplication::NotifyNodesThatIamClusterHead, this);
    }

    void ERGCApplication::clusterHeadMessageTimeout()
    {
        ClusterHeadSelectionHeader clusHeadSelectionH;
        clusHeadSelectionH.SetNodeId(getNodeId());
        clusHeadSelectionH.SetNodePosition(getNodePosition());
        clusHeadSelectionH.SetSCIndex(getSCIndex());
        clusHeadSelectionH.SetDistBtwNodeAndBS(getDistBtwNodeAndBS());
        clusHeadSelectionH.SetResidualEnrg(getResidualEnergy());
        clusHeadSelectionH.SetIsClusterHeadMsg(0);
        Ptr<Packet> packet = Create<Packet>(clusHeadSelectionH.GetSerializedSize());
        packet->AddHeader(clusHeadSelectionH);
        m_socket->Send(packet);
        m_lastStartTime = Simulator::Now();
    }

    void ERGCApplication::SendClusterHeadSelectionMsg()
    {
    }

    void ERGCApplication::HandleAddClusterNeighbor(Ptr<Packet> packet) {}

    void StartQueryingClusterNeighborPhase()
    {
        NS_LOG_DEBUG("Starting Cluster Neighbor Phase");
    }

    void ERGCApplication::StopApplication() // Called at time specified by Stop
    {
        NS_LOG_FUNCTION(this);

        // CancelEvents();
        if (m_socket != 0)
        {
            m_socket->Close();
        }
        else
        {
            NS_LOG_WARN("ERGCApplication found null socket to close in StopApplication");
        }
    }

    void ERGCApplication::NotifyNodesThatIamClusterHead()
    {
        // NS_LOG_INFO("value: " << m_clusterHead);
        if (!m_clusterHead)
        {
            return;
        }
        ClusterHeadSelectionHeader clusHeadSelectionH;
        clusHeadSelectionH.SetNodeId(getNodeId());
        clusHeadSelectionH.SetNodePosition(getNodePosition());
        clusHeadSelectionH.SetSCIndex(getSCIndex());
        clusHeadSelectionH.SetDistBtwNodeAndBS(getDistBtwNodeAndBS());
        clusHeadSelectionH.SetResidualEnrg(getResidualEnergy());
        clusHeadSelectionH.SetIsClusterHeadMsg(1);
        Ptr<Packet> packet = Create<Packet>(clusHeadSelectionH.GetSerializedSize());
        packet->AddHeader(clusHeadSelectionH);
        std::map<AquaSimAddress, ClusterHeadSelectionHeader>::iterator it;
        // NS_LOG_INFO("Cluster Nodes: " << m_clusterList.size());
        // NS_LOG_DEBUG(getNodeId() << " " << m_clusterList.size());
        for (it = m_clusterList.begin(); it != m_clusterList.end(); it++)
        {
            NS_LOG_INFO("Cluster Head Id: " << getNodeId() << "Cluster Internal Node Id: " << it->second.GetNodeId());
            Ptr<Socket> socket = Socket::CreateSocket(GetNode(), m_tid);
            int ret = -1;
            ret = socket->Bind();
            if (ret == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            PacketSocketAddress pSocket;
            pSocket.SetSingleDevice(0);
            pSocket.SetPhysicalAddress((Address)it->first);
            pSocket.SetProtocol(0);
            socket->Connect(pSocket);
            socket->SetAllowBroadcast(false);
            socket->SetConnectCallback(
                MakeCallback(&ERGCApplication::ConnectionSucceeded, this),
                MakeCallback(&ERGCApplication::ConnectionFailed, this));
            socket->SetRecvCallback(MakeCallback(&ERGCApplication::HandleRead, this));
            m_clusterSocketList[it->first] = socket;
            socket->Send(packet);
        }
    }
    // Event handlers

    // Private helpers
    void ERGCApplication::ConnectionSucceeded(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);
        m_connected = true;
    }

    void ERGCApplication::ConnectionFailed(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);
        NS_FATAL_ERROR("Can't connect");
    }
}