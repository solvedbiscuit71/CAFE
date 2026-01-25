#include "model/caf-context.hpp"
#include "caf-stack-helper.hpp"

#include "ndn-app-helper.hpp"
#include "ndn-stack-helper.hpp"
#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/string.h"
#include <string>

NS_LOG_COMPONENT_DEFINE("caf.StackHelper");

namespace ns3 {
namespace caf {

StackHelper::StackHelper() 
  : m_helper() {}

StackHelper::~StackHelper() {}

void
StackHelper::Install(NodeContainer nodes, bool SetDefaultRoutes, size_t maxCsSize)
{
  m_helper.SetDefaultRoutes(SetDefaultRoutes);
  m_helper.setCsSize(maxCsSize);
  m_helper.Install(nodes);

  for (NodeContainer::Iterator i = nodes.Begin(); i != nodes.End(); ++i) {
    Ptr<Node> node = *i;

    Ptr<Context> ctx = node->GetObject<caf::Context>();
    NS_ABORT_MSG_IF(!ctx, "CafContext must be aggregated to the node before calling CafStackHelper");
    
    switch (ctx->GetNodeType()) {
      case NODE_TYPE_VEHICLE:
        SetupVehicle(node);
        break;
      case NODE_TYPE_RSU:
        SetupRSU(node);
        break;
      case NODE_TYPE_BACKBONE:
        SetupBackbone(node);
        break;
      case NODE_TYPE_NONE:
      default:
        break;
    }
  }
}

void
StackHelper::SetupRSU(Ptr<Node> node)
{
  ndn::AppHelper helloProducer("ns3::ndn::HelloProducer");
  helloProducer.SetAttribute("Prefix", StringValue("/alert/hello/rsu/" + std::to_string(node->GetId())));

  ApplicationContainer apps = helloProducer.Install(node);
  // i.e. start time should alter to ensure consecutive RSUs don't fire at the same time
  apps.Start(Seconds(node->GetId() % 2 == 0 ? 0.0 : 0.005));

  NS_LOG_DEBUG("Installed HelloProducer on node(" << node->GetId() << ")");
}

void
StackHelper::SetupVehicle(Ptr<Node> node) 
{
  ndn::AppHelper helloConsumer("ns3::ndn::HelloConsumer");
  helloConsumer.SetAttribute("Prefix", StringValue("/alert/hello/rsu"));
  helloConsumer.SetAttribute("LifeTime", StringValue("60s"));

  ApplicationContainer apps = helloConsumer.Install(node);
  apps.Start(Seconds(0.0));

  NS_LOG_DEBUG("Installed HelloConsumer on node(" << node->GetId() << ")");
}

void
StackHelper::SetupBackbone(Ptr<Node> node)
{
}

} // namespace caf
} // namespace ns3