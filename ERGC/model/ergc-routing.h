/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Connecticut
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Robert Martin <robert.martin@engr.uconn.edu>
 */

#ifndef ERGC_ROUTING_H
#define ERGC_ROUTING_H

#include "ns3/aqua-sim-routing.h"
#include "ns3/aqua-sim-address.h"
#include "ns3/aqua-sim-datastructure.h"
#include "ns3/aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

#include <map>

namespace ns3
{

  class VBHeader;

  struct ergc_neighborhood
  {
    int number;
    Vector neighbor[MAX_NEIGHBOR];
  };

  typedef std::pair<AquaSimAddress, unsigned int> hash_entry;

  /**
   * \ingroup ergc
   *
   * \brief Packet Hash table for ERGC to assist in specialized tables.
   */
  class ERGCPktHashTable
  {
  public:
    std::map<hash_entry, ergc_neighborhood *> m_htable;
    // std::map<hash_t, hash_entry> m_htable;

    ERGCPktHashTable();
    ~ERGCPktHashTable();

    int m_windowSize;
    void Reset();
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum);
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p);
    ergc_neighborhood *GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
    // private:
    // int lower_counter;
  }; // class ERGCPktHashTable

  /**
   * \brief Packet Hash table for VBF to assist in specialized tables.
   */
  class DataHashTable
  {
  public:
    std::map<int *, int *> m_htable;
    // Tcl_HashTable htable;

    DataHashTable();
    ~DataHashTable();

    void Reset();
    void PutInHash(int *attr);
    int *GetHash(int *attr);
  }; // class DataHashTable

  /**
   * \brief ERGC Routing
   */
  class ERGCRouting : public AquaSimRouting
  {
  public:
    ERGCRouting();
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);

    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void SetTargetPos(Vector pos);
    // AquaSimVBF_Entry routing_table[MAX_DATA_TYPE];

  protected:
    int m_pkCount;
    int m_counter;
    int m_hopByHop;
    int m_enableRouting; // if true, VBF can perform routing functionality. Otherwise, not perform
    // int m_useOverhear;
    double m_priority;
    bool m_measureStatus;
    // int m_portNumber;
    ERGCPktHashTable PktTable;
    ERGCPktHashTable SourceTable;
    ERGCPktHashTable Target_discoveryTable;
    ERGCPktHashTable SinkTable;

    double m_width;
    // the width is used to test if the node is close enough to the path specified by the packet
    Vector m_targetPos;
    Ptr<UniformRandomVariable> m_rand;

    void Terminate();
    void Reset();
    void ConsiderNew(Ptr<Packet> pkt);
    void SetDelayTimer(Ptr<Packet>, double);
    void Timeout(Ptr<Packet>);
    double Advance(Ptr<Packet>);
    double Distance(Ptr<Packet>);
    double Projection(Ptr<Packet>);
    double CalculateDelay(Ptr<Packet>, Vector *);
    // double RecoveryDelay(Ptr<Packet>, Vector*);
    void CalculatePosition(Ptr<Packet>);
    void SetMeasureTimer(Ptr<Packet>, double);
    bool IsTarget(Ptr<Packet>);
    bool IsCloseEnough(Ptr<Packet>);

    Ptr<Packet> CreatePacket();
    Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type);

    void DataForSink(Ptr<Packet> pkt);
    void StopSource();
    void MACprepare(Ptr<Packet> pkt);
    void MACsend(Ptr<Packet> pkt, double delay = 0);

    virtual void DoDispose();
  }; // class AquaSimVBF

} // namespace ns3

#endif /* ERGC_ROUTING_H */
