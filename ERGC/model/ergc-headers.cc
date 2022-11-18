#include "ergc-headers.h"
#include "ns3/log.h"
#include "ns3/buffer.h"

using namespace ns3;

// ----------------------------------------------------------------
// cube length message
NS_LOG_COMPONENT_DEFINE("ergc-headers");
NS_OBJECT_ENSURE_REGISTERED(CubeLengthHeader);

CubeLengthHeader::CubeLengthHeader()
{
}

CubeLengthHeader::~CubeLengthHeader()
{
}

TypeId
CubeLengthHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CubeLengthHeader")
                            .SetParent<Header>()
                            .AddConstructor<CubeLengthHeader>();
    return tid;
}

uint32_t
CubeLengthHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_k_mtrs = i.ReadU32();
    return GetSerializedSize();
}

uint32_t
CubeLengthHeader::GetSerializedSize(void) const
{
    // reserved bytes for header
    return 4;
}

void CubeLengthHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU32(m_k_mtrs);
}

void CubeLengthHeader::Print(std::ostream &os) const
{
    os << "CubeLength header is: k_mtrs=" << m_k_mtrs << "\n";
}

TypeId
CubeLengthHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

uint32_t CubeLengthHeader::GetKMtrs()
{
    return m_k_mtrs;
}

void CubeLengthHeader::SetKMtrs(uint32_t k_mtrs)
{
    m_k_mtrs = k_mtrs;
}

// ----------------------------------------------------------------
// cluster head selection header
// NS_LOG_COMPONENT_DEFINE("ClusterHeadSelectionHeader");
NS_OBJECT_ENSURE_REGISTERED(ClusterHeadSelectionHeader);

ClusterHeadSelectionHeader::ClusterHeadSelectionHeader()
{
}

ClusterHeadSelectionHeader::~ClusterHeadSelectionHeader()
{
}

TypeId
ClusterHeadSelectionHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ClusterHeadSelectionHeader")
                            .SetParent<Header>()
                            .AddConstructor<ClusterHeadSelectionHeader>();
    return tid;
}

uint32_t
ClusterHeadSelectionHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_nodeId = (AquaSimAddress)i.ReadU16();
    m_nodePosition.x = ((double)i.ReadU32()) / 1000.0;
    m_nodePosition.y = ((double)i.ReadU32()) / 1000.0;
    m_nodePosition.z = ((double)i.ReadU32()) / 1000.0;
    m_scIndex.x = ((double)i.ReadU32()) / 1000.0;
    m_scIndex.y = ((double)i.ReadU32()) / 1000.0;
    m_scIndex.z = ((double)i.ReadU32()) / 1000.0;
    m_distBtwNodeAndBS = ((double)i.ReadU32()) / 1000.0;
    m_residualEnrg = ((double)i.ReadU32()) / 1000.0;
    return GetSerializedSize();
}

uint32_t
ClusterHeadSelectionHeader::GetSerializedSize(void) const
{
    // reserved bytes for header
    return (2 + 12 + 12 + 4 + 4);
}

void ClusterHeadSelectionHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU16(m_nodeId.GetAsInt());

    // node position
    i.WriteU32((uint32_t)(m_nodePosition.x * 1000.0 + 0.5)); //+0.5 for uint32_t typecast
    i.WriteU32((uint32_t)(m_nodePosition.y * 1000.0 + 0.5));
    i.WriteU32((uint32_t)(m_nodePosition.z * 1000.0 + 0.5));

    // sc index
    i.WriteU32((uint32_t)(m_scIndex.x * 1000.0 + 0.5)); //+0.5 for uint32_t typecast
    i.WriteU32((uint32_t)(m_scIndex.y * 1000.0 + 0.5));
    i.WriteU32((uint32_t)(m_scIndex.z * 1000.0 + 0.5));

    i.WriteU32((uint32_t)(m_distBtwNodeAndBS * 1000.0 + 0.5));
    i.WriteU32((uint32_t)(m_residualEnrg * 1000.0 + 0.5));
}

void ClusterHeadSelectionHeader::Print(std::ostream &os) const
{
    os << "Cluster Head Selection Header is: NodeId=" << m_nodeId
       << "NodePosition=" << m_nodePosition
       << " SCIndex=" << m_scIndex
       << " DistBtwNodeAndBS=" << m_distBtwNodeAndBS
       << " ResidualEnergy=" << m_residualEnrg
       << "\n";
}

TypeId
ClusterHeadSelectionHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

AquaSimAddress ClusterHeadSelectionHeader::GetNodeId()
{
    return m_nodeId;
}

Vector ClusterHeadSelectionHeader::GetNodePosition()
{
    return m_nodePosition;
}

Vector ClusterHeadSelectionHeader::GetSCIndex()
{
    return m_scIndex;
}

double ClusterHeadSelectionHeader::GetDistBtwNodeAndBS()
{
    return m_distBtwNodeAndBS;
}

double ClusterHeadSelectionHeader::GetResidualEnrg()
{
    return m_residualEnrg;
}

void ClusterHeadSelectionHeader::SetNodeId(AquaSimAddress nodeId)
{
    m_nodeId = nodeId;
}
void ClusterHeadSelectionHeader::SetNodePosition(Vector nodePosition)
{
    m_nodePosition = nodePosition;
}
void ClusterHeadSelectionHeader::SetSCIndex(Vector scIndex)
{
    m_scIndex = scIndex;
}
void ClusterHeadSelectionHeader::SetDistBtwNodeAndBS(double distBtwNodeAndBS)
{
    m_distBtwNodeAndBS = distBtwNodeAndBS;
}
void ClusterHeadSelectionHeader::SetResidualEnrg(double residualEnrg)
{
    m_residualEnrg = residualEnrg;
}

//------------------------------------------------------------------
// cluster neighbor header

NS_OBJECT_ENSURE_REGISTERED(ClusterNeighborHeader);

ClusterNeighborHeader::ClusterNeighborHeader()
{
}

ClusterNeighborHeader::~ClusterNeighborHeader()
{
}

TypeId
ClusterNeighborHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ClusterNeighborHeader")
                            .SetParent<Header>()
                            .AddConstructor<ClusterNeighborHeader>();
    return tid;
}

uint32_t
ClusterNeighborHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_clusterHeadId = (AquaSimAddress)i.ReadU16();
    m_nodePosition.x = ((double)i.ReadU32()) / 1000.0;
    m_nodePosition.y = ((double)i.ReadU32()) / 1000.0;
    m_nodePosition.z = ((double)i.ReadU32()) / 1000.0;
    m_scIndex.x = ((double)i.ReadU32()) / 1000.0;
    m_scIndex.y = ((double)i.ReadU32()) / 1000.0;
    m_scIndex.z = ((double)i.ReadU32()) / 1000.0;
    m_distBtwNodeAndBS = ((double)i.ReadU32()) / 1000.0;
    m_residualEnrg = ((double)i.ReadU32()) / 1000.0;
    return GetSerializedSize();
}

uint32_t
ClusterNeighborHeader::GetSerializedSize(void) const
{
    // reserved bytes for header
    return (2 + 12 + 12 + 4 + 4);
}

void ClusterNeighborHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU16(m_clusterHeadId.GetAsInt());

    // node position
    i.WriteU32((uint32_t)(m_nodePosition.x * 1000.0 + 0.5)); //+0.5 for uint32_t typecast
    i.WriteU32((uint32_t)(m_nodePosition.y * 1000.0 + 0.5));
    i.WriteU32((uint32_t)(m_nodePosition.z * 1000.0 + 0.5));

    // sc index
    i.WriteU32((uint32_t)(m_scIndex.x * 1000.0 + 0.5)); //+0.5 for uint32_t typecast
    i.WriteU32((uint32_t)(m_scIndex.y * 1000.0 + 0.5));
    i.WriteU32((uint32_t)(m_scIndex.z * 1000.0 + 0.5));

    i.WriteU32((uint32_t)(m_distBtwNodeAndBS * 1000.0 + 0.5));
    i.WriteU32((uint32_t)(m_residualEnrg * 1000.0 + 0.5));
}

void ClusterNeighborHeader::Print(std::ostream &os) const
{
    os << "Cluster Head Selection Header is: NodeId=" << m_clusterHeadId
       << "NodePosition=" << m_nodePosition
       << " SCIndex=" << m_scIndex
       << " DistBtwNodeAndBS=" << m_distBtwNodeAndBS
       << " ResidualEnergy=" << m_residualEnrg
       << "\n";
}

TypeId
ClusterNeighborHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

AquaSimAddress ClusterNeighborHeader::GetClusterHeadId()
{
    return m_clusterHeadId;
}

Vector ClusterNeighborHeader::GetNodePosition()
{
    return m_nodePosition;
}

Vector ClusterNeighborHeader::GetSCIndex()
{
    return m_scIndex;
}

double ClusterNeighborHeader::GetDistBtwNodeAndBS()
{
    return m_distBtwNodeAndBS;
}

double ClusterNeighborHeader::GetResidualEnrg()
{
    return m_residualEnrg;
}

void ClusterNeighborHeader::SetClusterHeadId(AquaSimAddress clusterHeadId)
{
    m_clusterHeadId = clusterHeadId;
}
void ClusterNeighborHeader::SetNodePosition(Vector nodePosition)
{
    m_nodePosition = nodePosition;
}
void ClusterNeighborHeader::SetSCIndex(Vector scIndex)
{
    m_scIndex = scIndex;
}
void ClusterNeighborHeader::SetDistBtwNodeAndBS(double distBtwNodeAndBS)
{
    m_distBtwNodeAndBS = distBtwNodeAndBS;
}
void ClusterNeighborHeader::SetResidualEnrg(double residualEnrg)
{
    m_residualEnrg = residualEnrg;
}