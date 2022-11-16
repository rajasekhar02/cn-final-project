#include "ergc-application-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ergc-application.h"

namespace ns3
{

    ERGCAppHelper::ERGCAppHelper(std::string protocol, Address address)
    {
        m_factory.SetTypeId("ns3::ERGCApplication");
        m_factory.Set("Protocol", StringValue(protocol));
        m_factory.Set("Remote", AddressValue(address));
    }

    void
    ERGCAppHelper::SetAttribute(std::string name, const AttributeValue &value)
    {
        m_factory.Set(name, value);
    }

    ApplicationContainer
    ERGCAppHelper::Install(Ptr<Node> node) const
    {
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    ERGCAppHelper::Install(std::string nodeName) const
    {
        Ptr<Node> node = Names::Find<Node>(nodeName);
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    ERGCAppHelper::Install(NodeContainer c) const
    {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
        {
            apps.Add(InstallPriv(*i));
        }

        return apps;
    }

    Ptr<Application>
    ERGCAppHelper::InstallPriv(Ptr<Node> node) const
    {
        Ptr<Application> app = m_factory.Create<Application>();
        node->AddApplication(app);

        return app;
    }

    void
    ERGCAppHelper::SetConstantRate(DataRate dataRate, uint32_t packetSize)
    {
        m_factory.Set("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1000]"));
        m_factory.Set("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        m_factory.Set("DataRate", DataRateValue(dataRate));
        m_factory.Set("PacketSize", UintegerValue(packetSize));
    }

} // namespace ns3
