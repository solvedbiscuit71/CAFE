#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"
#include "ns3/nstime.h"
#include "ns3/point-to-point-helper.h"

#include "ns3/simulator.h"
#include "scratch-utils.h"

#include "helper/caf-stack-helper.hpp"

#include "model/caf-context.hpp"
#include "model/caf-zor.hpp"

#include "apps/random-alert-producer.hpp"

#include <boost/date_time/dst_rules.hpp>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

namespace ns3 {

// ------------------------------------------------------------
// Middle placement spacing function
// ------------------------------------------------------------
double
h (double r, double w, double delta)
{
  return std::sqrt (4.0 * r * r - w * w) + delta;
}

void
InstallApplication(Ptr<Node> node, std::shared_ptr<caf::ZoR> zor, Time startTime, Time stopTime)
{
  // static variables are initialized only once
  static ndn::AppHelper producerHelper("ns3::ndn::RandomAlertProducer");
  producerHelper.SetAttribute("StartTime", TimeValue(startTime));
  producerHelper.SetAttribute("StopTime", TimeValue(stopTime));
  producerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/veh/" + std::to_string(node->GetId())));
  producerHelper.SetAttribute("Interval", StringValue("0s")); // only once
  producerHelper.SetAttribute("PayloadSize", UintegerValue(64));
  producerHelper.SetAttribute("Threshold", DoubleValue(0.05)); // 5% chance

  auto app = DynamicCast<ndn::RandomAlertProducer>(producerHelper.Install(node).Get(0));
  app->SetZoR(zor);
}

int
main (int argc, char *argv[])
{
  const double txRadius = 50.0;
  const double roadWidth = 21.0;
  const double maxRange = 400.0; // extend 400m in all directions

  std::string traceFile = "trace/scene2.tcl";

  // ------------------------------------------------------------
  // Command-line parameter
  // ------------------------------------------------------------
  double delta = 0.0;
  bool enableHello = false;
  bool disableRsu = false;
  double startTime = 60.0;
  double stopTime = 300.0;

  CommandLine cmd;
  cmd.AddValue ("delta", "Deviation from optimal spacing (default: 0.0)", delta);
  cmd.AddValue("enableHello", "Whether to enable hello messages or not (default: false)", enableHello);
  cmd.AddValue("disableRsu", "Whether to disable RSU or not (default: false)", disableRsu);
  cmd.AddValue("startTime", "Start time (default: 60s)", startTime);
  cmd.AddValue("stopTime", "Stop time (default: 300s)", stopTime);
  cmd.Parse (argc, argv);

  std::cout << "CLI arguments:" << '\n' 
            << "| delta="<<delta << '\n'
            << "| enableHello="<<enableHello << '\n'
            << "| disableRsu="<<disableRsu << '\n'
            << "| startTime="<<startTime << '\n'
            << "| stopTime="<<stopTime << std::endl;

  // ------------------------------------------------------------
  // Build animation filename
  // ------------------------------------------------------------
  // std::ostringstream oss;
  // oss << "netanim/scene2-" << std::fixed << std::setprecision (1) << delta << ".xml";

  std::string animFile = "netanim/scene2.xml";

  // ------------------------------------------------------------
  // Generate RSU positions
  // ------------------------------------------------------------
  std::vector<Vector> rsuPositions;

  // Centre node
  rsuPositions.emplace_back (Vector (0.0, 0.0, 0.0));

  double dx = h (txRadius, roadWidth, delta);
  int maxLayer = static_cast<int> (maxRange / dx);

  // +X direction
  for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back (Vector (i * dx, 0.0, 0.0));
  }

  // -X direction
  for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back (Vector (-i * dx, 0.0, 0.0));
  }

  // +Y direction
  for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back (Vector (0.0, i * dx, 0.0));
  }

  // -Y direction
  for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back (Vector (0.0, -i * dx, 0.0));
  }

  uint32_t numRsu = rsuPositions.size ();
  NodeContainer rsuNodes = createNodeAt (numRsu, rsuPositions);

  for (uint32_t i = 0; i < rsuPositions.size (); ++i) {
    std::cout << "Deploy rsu("<<i<<") at {"<<rsuPositions[i].x<<","<<rsuPositions[i].y<<"}" << std::endl;
  }

  // ------------------------------------------------------------
  // Mobility (vehicles)
  // ------------------------------------------------------------
  uint32_t numVehicles;
  double duration;
  NodeLifetime vehicleLifetime;

  ParseMobilityTrace (traceFile, numVehicles, vehicleLifetime, duration);

  NodeContainer vehicleNodes = createNodeWith (numVehicles, traceFile);
  

  // ------------------------------------------------------------
  // Set Context object
  // ------------------------------------------------------------
 
  caf::setupContext(rsuNodes, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_RSU);
    ctx->SetNodeStatus(caf::NODE_STATUS_ACTIVE);
  });

  caf::setupContext(vehicleNodes, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_VEHICLE);
    ctx->SetNodeStatus(caf::NODE_STATUS_ACTIVE);
  });

  // RSU#0 i.e. center RSU is under maintainence
  if (disableRsu) {
    rsuNodes.Get(0)->GetObject<caf::Context>()->SetNodeStatus(caf::NODE_STATUS_INACTIVE);
  }

  // ------------------------------------------------------------
  // Install NetDevice
  // ------------------------------------------------------------
  SetDefaultP2PConfig();
  PointToPointHelper p2p;
  
  for (int i=0; i<4; i++) {
    int k=1 + i * maxLayer;
    
    // RSU#0 i.e. center RSU is under maintainence
    if (!disableRsu) {
      p2p.Install(rsuNodes.Get(0), rsuNodes.Get(k));
    }
    for (int j=0; j<maxLayer-1; j++) {
      p2p.Install(rsuNodes.Get(k+j), rsuNodes.Get(k+j+1));
    }
  }

  NodeContainer adhocNodes;
  adhocNodes.Add(rsuNodes);
  adhocNodes.Add(vehicleNodes);
  SetupWifiNetDevice(adhocNodes);

  // ------------------------------------------------------------
  // Install CAFE stack
  // ------------------------------------------------------------
  caf::StackHelper stackHelper;
  stackHelper.setEnableHello(enableHello);
  stackHelper.setStartTime(Seconds(startTime));
  stackHelper.Install(rsuNodes);
  stackHelper.Install(vehicleNodes, true);

  // ------------------------------------------------------------
  // Install Application
  // ------------------------------------------------------------

  ndn::AppHelper consumerHelper("ns3::ndn::AlertConsumer");
  consumerHelper.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
  consumerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/veh"));
  consumerHelper.SetAttribute("LifeTime", StringValue("120s"));
  consumerHelper.Install(vehicleNodes);

  auto zor = std::make_shared<caf::CompositeZoR>();
  zor->append(std::make_unique<caf::CircleZoR>(caf::Point{0, 0}, 50.0));
  zor->append(std::make_unique<caf::PolygonZoR>(std::vector<caf::Point>{
    {10, 100},
    {10, -100},
    {-10, -100},
    {-10, 100},
  }));
  zor->append(std::make_unique<caf::PolygonZoR>(std::vector<caf::Point>{
    {100, 10},
    {100, -10},
    {-100, -10},
    {-100, 10},
  }));
  
  for (const auto& it: vehicleLifetime) {
    auto nodeId = it.first;
    auto inTime = it.second.first + 30.05; // +30.05seconds buffer time
    auto outTime = it.second.second;

    // skip node before saturation and after simulation
    if (inTime <= startTime || inTime >= stopTime) continue;

    // +numRsu because nodeId in vehicleLifetime starts from 0
    Ptr<Node> node = vehicleNodes.Get(nodeId + numRsu);
    if (node == nullptr)
      continue;
    
    Ptr<caf::Context> ctx = node->GetObject<caf::Context>();
    if (!ctx || ctx->GetNodeType() != caf::NODE_TYPE_VEHICLE)
      continue;

    InstallApplication(node, zor, Seconds(inTime), Seconds(outTime));
    std::cout << "Install application on vehicle("<< node->GetId() <<") at="<<inTime<<"s" << std::endl;
  }

  // ------------------------------------------------------------
  // NetAnim
  // ------------------------------------------------------------
  AnimationInterface anim (animFile);
  if (enableHello) {
    setNodesColor(anim, rsuNodes, 0, 255, 0); // default: green (active)
  } else {
    setNodesColor(anim, rsuNodes, 255, 0, 0); // red (inactive)
  }
  if (disableRsu) {
    setNodesColor(anim, rsuNodes.Get(0), 255, 0, 0); // red (inactive)
  }
  setNodesColor(anim, vehicleNodes, 0, 0, 255); // default: blue

  Simulator::Stop (Seconds (stopTime));
  std::cout << "Simulator duration set to " << stopTime << " seconds" << std::endl;
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

} // namespace ns3

int
main (int argc, char *argv[])
{
  return ns3::main (argc, argv);
}
