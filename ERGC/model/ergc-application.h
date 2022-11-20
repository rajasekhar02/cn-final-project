
#ifndef ONOFF_APPLICATION_H
#define ONOFF_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/seq-ts-size-header.h"
#include "ergc-headers.h"
namespace ns3
{

    class Address;
    class RandomVariableStream;
    class Socket;
    class ERGCApplication : public Application
    {
    public:
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        ERGCApplication();

        virtual ~ERGCApplication();

        /**
         * \brief Set the total number of bytes to send.
         *
         * Once these bytes are sent, no packet is sent again, even in on state.
         * The value zero means that there is no limit.
         *
         * \param maxBytes the total number of bytes to send
         */
        void SetMaxBytes(uint64_t maxBytes);

        /**
         * \brief Return a pointer to associated socket.
         * \return pointer to associated socket
         */
        Ptr<Socket> GetSocket(void) const;

    protected:
        virtual void DoDispose(void);

    private:
        // inherited from Application base class.
        virtual void StartApplication(void); // Called at time specified by Start
        virtual void StopApplication(void);  // Called at time specified by Stop

        // helpers
        void
        isClusterHead();
        void
        handleBSStartEvent();

        void
        handleNodeStartEvent();

        void
        SendPacketWithK();

        void
        HandleRead(Ptr<Socket> socket);

        void
        HandleCubeLengthAssignPacket(Ptr<Packet> &packet);

        void
        StartClusteringPhase();

        void
        ScheduleClusterHeadSelectionPhase();

        void
        clusterHeadMessageTimeout();

        void
        HandleClusterHeadSelection(Ptr<Packet> packet);

        void
        StartQueryingClusterNeighborPhase();

        void
        HandleAddClusterNeighbor(Ptr<Packet> packet);

        void
        SendClusterHeadSelectionMsg();
        /**
         * \brief Cancel all pending events.
         */
        void
        CancelEvents();

        // Event handlers
        /**
         * \brief Start an On period
         */
        void StartSending();
        /**
         * \brief Start an Off period
         */
        void StopSending();
        /**
         * \brief Send a packet
         */
        void SendPacket();

        Ptr<Socket> m_socket;                //!< Associated socket
        Address m_peer;                      //!< Peer address
        Address m_local;                     //!< Local address to bind to
        bool m_connected;                    //!< True if connected
        Ptr<RandomVariableStream> m_onTime;  //!< rng for On Time
        Ptr<RandomVariableStream> m_offTime; //!< rng for Off Time
        DataRate m_cbrRate;                  //!< Rate that data is generated
        DataRate m_cbrRateFailSafe;          //!< Rate that data is generated (check copy)
        uint32_t m_pktSize;                  //!< Size of packets
        uint32_t m_residualBits;             //!< Number of generated, but not sent, bits
        Time m_lastStartTime;                //!< Time last packet sent
        uint64_t m_maxBytes;                 //!< Limit total number of bytes sent
        uint64_t m_totBytes;                 //!< Total bytes sent so far
        TypeId m_tid;                        //!< Type of the socket used
        uint32_t m_seq{0};                   //!< Sequence
        Ptr<Packet> m_unsentPacket;          //!< Unsent packet cached for future attempt
        bool m_enableSeqTsSizeHeader{false}; //!< Enable or disable the use of SeqTsSizeHeader
        // ERGC Algorithm Variables
        EventId m_BSSentKEvent;                                               //!< Event id for BS sent k
        EventId m_sendClusMsgEvent;                                           //!< Event id of cluster msg event
        bool m_clusterHead{true};                                             //!< before cluster head selection every node is a cluster head
        std::map<AquaSimAddress, ClusterHeadSelectionHeader> clusterList;     // key -> address of the child node of this cluster header
        std::map<AquaSimAddress, ClusterNeighborHeader> neighborClusterTable; // key -> address of the neighbor cluster header
        Time m_maxClusterHeadSelectionTime{Time(10)};
        /// Traced Callback: transmitted packets.
        TracedCallback<Ptr<const Packet>> m_txTrace;

        /// Callbacks for tracing the packet Tx events, includes source and destination addresses
        TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_txTraceWithAddresses;

        /// Callback for tracing the packet Tx events, includes source, destination, the packet sent, and header
        TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader &> m_txTraceWithSeqTsSize;

    private:
        /**
         * \brief Handle a Connection Succeed event
         * \param socket the connected socket
         */
        void ConnectionSucceeded(Ptr<Socket> socket);
        /**
         * \brief Handle a Connection Failed event
         * \param socket the not connected socket
         */
        void ConnectionFailed(Ptr<Socket> socket);
    };

} // namespace ns3

#endif /* ONOFF_APPLICATION_H */
