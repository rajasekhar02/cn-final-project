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

#ifndef AQUA_SIM_ROUTING_STATIC_H
#define AQUA_SIM_ROUTING_STATIC_H

#include "ns3/aqua-sim-routing.h"
#include "ns3/aqua-sim-address.h"
#include "ergc-headers.h"
#include <map>

namespace ns3
{

/*header length of Static routing*/
#define SR_HDR_LEN (3 * sizeof(AquaSimAddress) + sizeof(int))

  /**
   * \ingroup aqua-sim-ng
   *
   * \brief Static routing implementation
   */
  class ERGCRouting : public AquaSimRouting
  {
  public:
    bool m_is_cluster_head{false};
    AquaSimAddress m_cluster_head_address;
    ERGCRouting();
    ERGCRouting(char *routeFile);
    virtual ~ERGCRouting();
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);

    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void SetRouteTable(char *routeFile);

  protected:
    bool m_hasSetRouteFile;
    bool m_hasSetNode;
    char m_routeFile[100];
    std::map<AquaSimAddress, ClusterNeighborHeader> m_neighborClusterTable;
    void ReadRouteTable(char *filename);
    AquaSimAddress FindNextHop(const Ptr<Packet> p);

  private:
    std::map<AquaSimAddress, AquaSimAddress> m_rTable;

  }; // class ERGCRouting

} // namespace ns3

#endif /* AQUA_SIM_ROUTING_STATIC_H */
