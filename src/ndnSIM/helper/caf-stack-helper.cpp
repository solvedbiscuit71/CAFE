#include "model/caf-context.hpp"
#include "caf-stack-helper.hpp"

#include "ndn-app-helper.hpp"
#include "ndn-stack-helper.hpp"
#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/nstime.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include <cstdint>
#include <string>

NS_LOG_COMPONENT_DEFINE("caf.StackHelper");

namespace ns3 {
namespace caf {

StackHelper::StackHelper() 
  : m_helper(), m_enableHello(false) {}

StackHelper::~StackHelper() {}


bool
StackHelper::getEnableHello() { return m_enableHello; }

void
StackHelper::setEnableHello(bool enableHello) { m_enableHello = enableHello; }

void
StackHelper::Install(NodeContainer nodes, bool SetDefaultRoutes, size_t maxCsSize)
{
  m_helper.SetDefaultRoutes(SetDefaultRoutes);
  m_helper.setCsSize(maxCsSize);

  for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it) {
    Ptr<Node> node = *it;

    Ptr<Context> ctx = node->GetObject<caf::Context>();
    NS_ABORT_MSG_IF(!ctx, "CafContext must be aggregated to the node before calling CafStackHelper");
    
    // skip setup on inactive nodes
    if (ctx && ctx->GetNodeStatus() == caf::NODE_STATUS_INACTIVE) {
      NS_LOG_DEBUG("Skip setup on node("<< node->GetId() <<")");
      continue;
    }

    // install L3Protocol stack
    m_helper.Install(node);
    
    // install application
    switch (ctx->GetNodeType()) {
      case NODE_TYPE_VEHICLE:
        SetupVehicle(node, ctx);
        break;
      case NODE_TYPE_RSU:
        SetupRSU(node, ctx);
        break;
      case NODE_TYPE_BACKBONE:
        SetupBackbone(node, ctx);
        break;
      case NODE_TYPE_NONE:
      default:
        break;
    }
  }
}

double
calculateIRTF(double payloadSize, double headerSize=226, double txRate=3e6)
{
  // headerSize=226 based on empirical evidence
  // IEEE802.11p standard uses txRate=3e6 (3Mbps) 
  // +40μs buffer for propagation, preamble, and header
  return (headerSize + payloadSize) * 8 / txRate + 40e-6;
}

void
StackHelper::SetupRSU(Ptr<Node> node, Ptr<Context> ctx)
{
  // configure node position (RSU position is static)
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
  if (mobility != nullptr) {
    Vector v = mobility->GetPosition();

    auto P = ctx->GetPositionInfo();
    (*P)[node->GetId()] = Point{static_cast<float>(v.x), static_cast<float>(v.y)};
  }

  // guard condition
  if (!m_enableHello)
    return;

  /**
   * BSM (Basic Safety Message) are typically very small 50-100 bytes.
   * Given, current use case doesn't use the data content, we will restrict it
   * to 64 bytes.
   */
  double payloadSize = 64;

  /**
   * Even RSU are phase 0ms while odd RSU are phase 50ms
   * which means
   * even RSU := 0,    200,    400,    600, ...
   * odd RSU :=    100,    300,    500,    700, ...
   */
  auto startTime_in_ms = MilliSeconds(1000 + (node->GetId() % 2 == 0 ? 0 : 100)); // start from 1sec = 1000ms

  ndn::AppHelper helloProducer("ns3::ndn::HelloProducer");
  helloProducer.SetAttribute("StartTime", TimeValue(startTime_in_ms));
  helloProducer.SetAttribute("Prefix", StringValue("/alert/hello/rsu/" + std::to_string(node->GetId())));
  helloProducer.SetAttribute("Interval", StringValue("200ms"));
  helloProducer.SetAttribute("PayloadSize", UintegerValue(payloadSize));

  ApplicationContainer apps = helloProducer.Install(node);

  NS_LOG_DEBUG("Installed HelloProducer on node(" << node->GetId() << ")");
}

void
StackHelper::SetupVehicle(Ptr<Node> node, Ptr<Context> ctx) 
{
  // guard condition
  if (!m_enableHello)
    return;

  ndn::AppHelper helloConsumer("ns3::ndn::HelloConsumer");
  helloConsumer.SetAttribute("StartTime", StringValue("1s"));
  helloConsumer.SetAttribute("Prefix", StringValue("/alert/hello/rsu"));
  helloConsumer.SetAttribute("LifeTime", StringValue("500ms"));

  ApplicationContainer apps = helloConsumer.Install(node);

  NS_LOG_DEBUG("Installed HelloConsumer on node(" << node->GetId() << ")");
}

void
StackHelper::SetupBackbone(Ptr<Node> node, Ptr<Context> ctx)
{
}

} // namespace caf
} // namespace ns3