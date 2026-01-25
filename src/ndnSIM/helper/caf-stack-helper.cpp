#include "caf-stack-helper.hpp"
#include "model/ndn-context.hpp"
#include "src/ndnSIM/helper/ndn-stack-helper.hpp"

namespace ns3 {
namespace caf {

CafStackHelper::CafStackHelper() 
  : m_helper() {}

CafStackHelper::~CafStackHelper() {}

void
CafStackHelper::Install(NodeContainer nodes, bool SetDefaultRoutes, size_t maxCsSize)
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
CafStackHelper::SetupRSU(Ptr<Node> node)
{

}

void
CafStackHelper::SetupVehicle(Ptr<Node> node) 
{

}

void
CafStackHelper::SetupBackbone(Ptr<Node> node)
{

}

} // namespace caf
} // namespace ns3