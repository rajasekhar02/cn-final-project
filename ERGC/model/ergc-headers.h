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

#ifndef ERGC_HEADER_ROUTING_H
#define ERGC_HEADER_ROUTING_H

// #include <string>
#include <iostream>

#include "ns3/header.h"
#include "ns3/vector.h"

#include "ns3/aqua-sim-address.h"
#include "ns3/aqua-sim-datastructure.h"

#define DBRH_DATA_GREEDY 0
#define DBRH_DATA_RECOVER 1
#define DBRH_BEACON 2

namespace ns3
{

    /**
     * \ingroup ergc
     *
     * \brief Dynamic routing header
     * Cluster-Msg includes
     * node identifier,
     * node coordinates (x,y,z),
     * distance to node and BS (because node is moving)
     * SC identifier Gi(m,n,h)
     * the residual energy Eres.
     */
    class ClusterHeadSelectionHeader : public Header
    {
    public:
        ClusterHeadSelectionHeader();
        virtual ~ClusterHeadSelectionHeader();
        static TypeId GetTypeId();

        AquaSimAddress GetNodeId();
        Vector GetNodePosition();
        Vector GetSCIndex();
        double GetDistBtwNodeAndBS();
        double GetResidualEnrg();

        void SetNodeId(AquaSimAddress nodeId);
        void SetNodePosition(Vector nodePosition);
        void SetSCIndex(Vector scIndex);
        void SetDistBtwNodeAndBS(double distBtwNodeAndBS);
        void SetResidualEnrg(double residualEnrg);

        // inherited methods
        virtual uint32_t GetSerializedSize(void) const;
        virtual void Serialize(Buffer::Iterator start) const;
        virtual uint32_t Deserialize(Buffer::Iterator start);
        virtual void Print(std::ostream &os) const;
        virtual TypeId GetInstanceTypeId(void) const;

    private:
        AquaSimAddress m_nodeId;
        Vector m_nodePosition;
        Vector m_scIndex;
        double m_distBtwNodeAndBS;
        double m_residualEnrg;
    }; // class ClusterHeadSelectionHeader

} // namespace ns3

#endif /* ERGC_HEADER_ROUTING_H */
