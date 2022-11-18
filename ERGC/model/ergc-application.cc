
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
        : m_socket(0),
          m_connected(false),
          m_residualBits(0),
          m_lastStartTime(Seconds(0)),
          m_totBytes(0),
          m_unsentPacket(0)
    {
        NS_LOG_FUNCTION(this);
    }

    ERGCApplication::~ERGCApplication()
    {
        NS_LOG_FUNCTION(this);
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

    void
    ERGCApplication::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);

        CancelEvents();
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
            GetNode()->GetDevice(0)->AggregateObject(m_socket);
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
        std::string nodeType = GetNode()->GetObject<ERGCNodeProps>()->nodeType;
        if (nodeType == "BS")
        {
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
        NS_ASSERT(m_sendEvent.IsExpired());
        u_int32_t k_mtrs = GetNode()->GetObject<ERGCNodeProps>()->k_mtrs;
        CubeLengthHeader cubeLengthHeader;
        cubeLengthHeader.SetKMtrs(k_mtrs);
        Ptr<Packet> packet = Create<Packet>(cubeLengthHeader.GetSerializedSize());
        packet->AddHeader(cubeLengthHeader);
        m_socket->Send(packet);
        m_lastStartTime = Simulator::Now();
    }

    void 
    ERGCApplication::HandleRead(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);
        Ptr<Packet> packet;
        Address from;
        Address localAddress;
        while ((packet = socket->Recv()))
        {
            socket->GetSockName(localAddress);
            if (packet->GetSize() > 0)
            {
                CubeLengthHeader cubeLengthTs;
                if (packet->PeekHeader(cubeLengthTs))
                {
                    packet->RemoveHeader(cubeLengthTs);
                    std::cout << cubeLengthTs.GetKMtrs() << std::endl;
                    Ptr<ERGCNodeProps> ergcNodeProps= GetNode()->GetObject<ERGCNodeProps>();
                    GetNode()->GetObject<ERGCNodeProps>()->k_mtrs = cubeLengthTs.GetKMtrs();

                }
            }
        }
    }

    void 
    ERGCApplication::StopApplication() // Called at time specified by Stop
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

    void 
    ERGCApplication::CancelEvents()
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
        Simulator::Cancel(m_BSSentKEvent);
        // Canceling events may cause discontinuity in sequence number if the
        // SeqTsSizeHeader is header, and m_unsentPacket is true
        if (m_unsentPacket)
        {
            NS_LOG_DEBUG("Discarding cached packet upon CancelEvents ()");
        }
        m_unsentPacket = 0;
    }

    // Event handlers
   
    // Private helpers
    void 
    ERGCApplication::ConnectionSucceeded(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);
        m_connected = true;
    }

    void 
    ERGCApplication::ConnectionFailed(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);
        NS_FATAL_ERROR("Can't connect");
    }
}