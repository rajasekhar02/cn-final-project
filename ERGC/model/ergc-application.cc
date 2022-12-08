
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
#include "ergc-routing.h"

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

  AquaSimAddress ERGCApplication::getBaseStationAddress()
  {
    Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
    return AquaSimAddress::ConvertFrom(ergcNodeProps->m_BSAddress);
  }

  AquaSimAddress ERGCApplication::getNodeId()
  {
    return AquaSimAddress::ConvertFrom(GetNode()->GetDevice(0)->GetAddress());
  }

  ClusterHeadSelectionHeader ERGCApplication::getClusterHeadInfo()
  {
    return m_clusterHeadInfo;
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
    if (!m_clus_socket)
    {
      m_clus_socket = Socket::CreateSocket(GetNode(), m_tid);
      int ret = -1;
      ret = m_clus_socket->Bind();
      if (ret == -1)
      {
        NS_FATAL_ERROR("Failed to bind socket");
      }
      m_clus_socket->Connect(m_peer);
      m_clus_socket->SetAllowBroadcast(true);
      m_clus_socket->SetConnectCallback(
          MakeCallback(&ERGCApplication::ConnectionSucceeded, this),
          MakeCallback(&ERGCApplication::ConnectionFailed, this));
    }
    m_clus_socket->SetRecvCallback(MakeCallback(&ERGCApplication::HandleRead, this));
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
    m_clus_socket->Send(packet);
    m_lastStartTime = Simulator::Now();
    NS_LOG_INFO("BS Broadcasted k: " << k_mtrs << " " << m_lastStartTime << '\n');
  }

  // clustering phase methods
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
      if (packet->PeekHeader(cubeLengthTs) && !m_receivedK)
      {
        HandleCubeLengthAssignPacket(packet);
        packet = 0;
        StartClusteringPhase();
        return;
      }

      if (packet->PeekHeader(clusterHSH) && !m_completedClusHeadSelection)
      {
        HandleClusterHeadSelection(packet);
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
      NS_LOG_DEBUG("Node Id: " << getNodeId()
                               << " Ci " << Ci
                               << " Node j Id: "
                               << clusHeadSelectionH.GetNodeId()
                               << " Cj " << Cj
                               << " " << m_sendClusMsgEvent.GetTs()
                               << " " << Simulator::Now());
      if (m_sendClusMsgEvent.IsRunning())
      {
        NS_LOG_DEBUG("Listening for cluster initialization msg");
        if (Ci < Cj)
        {
          m_clusterHead = false;
          NS_LOG_DEBUG("Not a cluster Head: " << getNodeId());
          Simulator::Cancel(m_sendClusMsgEvent);
        }
        else
        {
          m_clusterList[clusHeadSelectionH.GetNodeId()] = clusHeadSelectionH;
          NS_LOG_DEBUG("Cluster Size: " << m_clusterList.size());
        }
        return;
      }
      m_clusterHead = false;
      m_clusterHeadInfo = clusHeadSelectionH;
      NS_LOG_DEBUG("Cluster Head Node " << m_clusterHeadInfo.GetNodeId() << " Cluster Head SC Index " << clusHeadSelectionH.GetSCIndex());
    }
  }

  void ERGCApplication::HandleAddClusterNeighbor(Ptr<Packet> packet)
  {
    if (!m_clusterHead)
    {
      return;
    }
    ClusterNeighborHeader clusNeighborH;
    packet->RemoveHeader(clusNeighborH);
    double dist = ERGCNodeProps::distanceBTW(getNodePosition(), clusNeighborH.GetNodePosition());
    NS_LOG_DEBUG("Received Message from Cluster Neighbor: " << getSCIndex()
                                                            << " "
                                                            << getDistBtwNodeAndBS()
                                                            << "---" << clusNeighborH.GetSCIndex()
                                                            << " " << (clusNeighborH.GetDistBtwNodeAndBS())
                                                            << " " << (dist)
                                                            << " " << getSqrt6Dist());
    if (getSCIndex() < clusNeighborH.GetSCIndex())
      return;
    m_neighborClusterTable[clusNeighborH.GetClusterHeadId()] = clusNeighborH;
    Ptr<AquaSimNetDevice> device = GetNode()->GetDevice(0)->GetObject<AquaSimNetDevice>();
    Ptr<ERGCRouting> ergcRouting = device->GetRouting()->GetObject<ERGCRouting>();
    ergcRouting->m_neighbor_cluster_table[clusNeighborH.GetClusterHeadId()] = clusNeighborH;
    NS_LOG_DEBUG(m_neighborClusterTable.size());
  }

  void ERGCApplication::StartClusteringPhase()
  {
    m_clusterHead = true;
    Ptr<ERGCNodeProps> ergcNodeProps = GetNode()->GetObject<ERGCNodeProps>();
    ergcNodeProps->m_scIndex = ERGCNodeProps::SCIndex(getNodePosition(), getK());
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
    NS_LOG_INFO("Timeout time: " << (m_broadcastClusHeadTimeOut).GetDouble());
    Ptr<AquaSimNetDevice> device = GetNode()->GetDevice(0)->GetObject<AquaSimNetDevice>();
    device->GetPhy()->SetTransRange(getSqrt3Dist());
    NS_LOG_DEBUG("Node Id: " << getNodeId()
                             << " SC Index " << getSCIndex()
                             << " At: " << Simulator::Now()
                             << " Timeout Time: " << m_broadcastClusHeadTimeOut
                             << " Distance btw node to BS: " << ergcNodeProps->distanceBTW(nodePosition, Vector(0, 0, 0))
                             << " 2 * m_k_mtrs" << 2 * getK());
    // m_broadcastClusHeadStartTime = Simulator::Now();
    m_sendClusMsgEvent = Simulator::Schedule(m_broadcastClusHeadTimeOut, &ERGCApplication::clusterHeadMessageTimeout, this);
    Simulator::Schedule(m_maxClusterHeadSelectionTime + m_broadcastClusHeadTimeOut, &ERGCApplication::StartQueryingClusterNeighborPhase, this);
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
    m_clus_socket->Send(packet);
    m_lastStartTime = Simulator::Now();
  }

  void ERGCApplication::StartQueryingClusterNeighborPhase()
  {
    m_completedClusHeadSelection = true;
    Ptr<AquaSimNetDevice> device = GetNode()->GetDevice(0)->GetObject<AquaSimNetDevice>();
    Ptr<ERGCRouting> ergcRouting = device->GetRouting()->GetObject<ERGCRouting>();
    ergcRouting->m_is_cluster_head = m_clusterHead;
    if (!m_clusterHead)
    {
      NS_LOG_DEBUG("Started sending data packets");
      ergcRouting->m_cluster_head_address = m_clusterHeadInfo.GetNodeId();
      device->GetPhy()->SetTransRange(getSqrt3Dist());
      Simulator::Schedule(Time("10s"), &ERGCApplication::initDataSocket, this);
      // initDataSocket();
      // Ptr<AquaSimNetDevice> device = GetNode()->GetDevice(0)->GetObject<AquaSimNetDevice>();
      return;
    }
    m_clusterHead = true;
    // NS_LOG_DEBUG(getSCIndex() << " " << m_clusterList.size());
    NS_LOG_DEBUG("Sending Message from Cluster Neighbor: " << getSCIndex()
                                                           << " "
                                                           << getDistBtwNodeAndBS());
    NS_LOG_DEBUG("Starting Cluster Neighbor Phase: " << getNodeId());
    device->GetPhy()->SetTransRange(getSqrt6Dist());
    ClusterNeighborHeader clusNeighborH;
    clusNeighborH.SetClusterHeadId(getNodeId());
    clusNeighborH.SetNodePosition(getNodePosition());
    clusNeighborH.SetSCIndex(getSCIndex());
    clusNeighborH.SetDistBtwNodeAndBS(getDistBtwNodeAndBS());
    clusNeighborH.SetResidualEnrg(getResidualEnergy());
    Ptr<Packet> packet = Create<Packet>(clusNeighborH.GetSerializedSize());
    packet->AddHeader(clusNeighborH);
    m_clus_socket->Send(packet);
    m_lastStartTime = Simulator::Now();
  }
  void ERGCApplication::initDataSocket()
  {

    if (!m_socket)
    {
      m_socket = Socket::CreateSocket(GetNode(), m_tid);
      int ret = -1;
      PacketSocketAddress peerSocket;
      // peerSocket.SetPhysicalAddress();
      peerSocket.SetSingleDevice(0);
      peerSocket.SetPhysicalAddress((Address)getBaseStationAddress());
      peerSocket.SetProtocol(0);
      if (InetSocketAddress::IsMatchingType(peerSocket) ||
          PacketSocketAddress::IsMatchingType(peerSocket))
      {
        ret = m_socket->Bind();
      }

      if (ret == -1)
      {
        NS_FATAL_ERROR("Failed to bind socket");
      }

      m_socket->Connect(peerSocket);
      m_socket->SetAllowBroadcast(false);
      m_socket->ShutdownRecv();

      m_socket->SetConnectCallback(
          MakeCallback(&ERGCApplication::ConnectionSucceeded, this),
          MakeCallback(&ERGCApplication::ConnectionFailed, this));
    }
    m_cbrRateFailSafe = m_cbrRate;

    // Insure no pending event
    CancelEvents();
    // If we are not yet connected, there is nothing to do here
    // The ConnectionComplete upcall will start timers at that time
    // if (!m_connected) return;
    ScheduleStartEvent();
  }
  // send normal message methods
  void ERGCApplication::StopApplication() // Called at time specified by Stop
  {
    NS_LOG_FUNCTION(this);

    CancelEvents();
    if (m_socket != 0)
    {
      m_socket->Close();
    }
    else
    {
      NS_LOG_WARN("ERGCApplication found null socket to close in StopApplication");
    }
  }

  void ERGCApplication::CancelEvents()
  {
    NS_LOG_FUNCTION(this);

    if (m_sendEvent.IsRunning() && m_cbrRateFailSafe == m_cbrRate)
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta(Simulator::Now() - m_lastStartTime);
      int64x64_t bits = delta.To(Time::S) * m_cbrRate.GetBitRate();
      m_residualBits += bits.GetHigh();
    }
    m_cbrRateFailSafe = m_cbrRate;
    Simulator::Cancel(m_sendEvent);
    Simulator::Cancel(m_startStopEvent);
    // Canceling events may cause discontinuity in sequence number if the
    // SeqTsSizeHeader is header, and m_unsentPacket is true
    if (m_unsentPacket)
    {
      NS_LOG_DEBUG("Discarding cached packet upon CancelEvents ()");
    }
    m_unsentPacket = 0;
  }

  // Event handlers
  void ERGCApplication::StartSending()
  {
    NS_LOG_FUNCTION(this);
    m_lastStartTime = Simulator::Now();
    ScheduleNextTx(); // Schedule the send packet event
    ScheduleStopEvent();
  }

  void ERGCApplication::ScheduleNextTx()
  {
    NS_LOG_FUNCTION(this);

    if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      NS_ABORT_MSG_IF(m_residualBits > m_pktSize * 8, "Calculation to compute next send time will overflow");
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC("bits = " << bits);
      Time nextTime(Seconds(bits /
                            static_cast<double>(m_cbrRate.GetBitRate()))); // Time till next packet
      NS_LOG_LOGIC("nextTime = " << nextTime.As(Time::S));
      m_sendEvent = Simulator::Schedule(nextTime,
                                        &ERGCApplication::SendPacket, this);
    }
    else
    { // All done, cancel any pending events
      StopApplication();
    }
  }

  void ERGCApplication::ScheduleStartEvent()
  { // Schedules the event to start sending data (switch to the "On" state)
    NS_LOG_FUNCTION(this);

    Time offInterval = Seconds(m_offTime->GetValue());
    NS_LOG_LOGIC("start at " << offInterval.As(Time::S));
    m_startStopEvent = Simulator::Schedule(offInterval, &ERGCApplication::StartSending, this);
  }

  void ERGCApplication::SendPacket()
  {
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_sendEvent.IsExpired());

    Ptr<Packet> packet;
    if (m_unsentPacket)
    {
      packet = m_unsentPacket;
    }
    else if (m_enableSeqTsSizeHeader)
    {
      Address from, to;
      m_socket->GetSockName(from);
      m_socket->GetPeerName(to);
      SeqTsSizeHeader header;
      header.SetSeq(m_seq++);
      header.SetSize(m_pktSize);
      NS_ABORT_IF(m_pktSize < header.GetSerializedSize());
      packet = Create<Packet>(m_pktSize - header.GetSerializedSize());
      // Trace before adding header, for consistency with PacketSink
      m_txTraceWithSeqTsSize(packet, from, to, header);
      packet->AddHeader(header);
    }
    else
    {
      packet = Create<Packet>(m_pktSize);
    }

    int actual = m_socket->Send(packet);
    if ((unsigned)actual == m_pktSize)
    {
      m_txTrace(packet);
      m_totBytes += m_pktSize;
      m_unsentPacket = 0;
      Address localAddress;
      m_socket->GetSockName(localAddress);
      if (InetSocketAddress::IsMatchingType(m_peer))
      {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S)
                               << " on-off application sent "
                               << packet->GetSize() << " bytes to "
                               << InetSocketAddress::ConvertFrom(m_peer).GetIpv4()
                               << " port " << InetSocketAddress::ConvertFrom(m_peer).GetPort()
                               << " total Tx " << m_totBytes << " bytes");
        m_txTraceWithAddresses(packet, localAddress, InetSocketAddress::ConvertFrom(m_peer));
      }
      else if (Inet6SocketAddress::IsMatchingType(m_peer))
      {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S)
                               << " on-off application sent "
                               << packet->GetSize() << " bytes to "
                               << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6()
                               << " port " << Inet6SocketAddress::ConvertFrom(m_peer).GetPort()
                               << " total Tx " << m_totBytes << " bytes");
        m_txTraceWithAddresses(packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
      }
    }
    else
    {
      NS_LOG_DEBUG("Unable to send packet; actual " << actual << " size " << m_pktSize << "; caching for later attempt");
      m_unsentPacket = packet;
    }
    m_residualBits = 0;
    m_lastStartTime = Simulator::Now();
    ScheduleNextTx();
  }

  void ERGCApplication::ScheduleStopEvent()
  { // Schedules the event to stop sending data (switch to "Off" state)
    NS_LOG_FUNCTION(this);

    Time onInterval = Seconds(m_onTime->GetValue());
    NS_LOG_LOGIC("stop at " << onInterval.As(Time::S));
    m_startStopEvent = Simulator::Schedule(onInterval, &ERGCApplication::StopSending, this);
  }

  void ERGCApplication::StopSending()
  {
    NS_LOG_FUNCTION(this);
    CancelEvents();
    ScheduleStartEvent();
  }

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