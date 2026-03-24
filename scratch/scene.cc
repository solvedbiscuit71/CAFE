#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/point-to-point-helper.h"

#include "ns3/random-variable-stream.h"
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
h (double r, double w)
{
  return std::sqrt (4.0 * r * r - w * w);
}

void ShowProgress(double start, double stop) {
  double now = Simulator::Now().GetSeconds();
  double progress = (now - start) / (stop - start);

  // Ensure progress doesn't exceed 100% due to precision
  if (progress > 1.0) progress = 1.0;

  int barWidth = 50;
  std::cout << "\r[";
  int pos = barWidth * progress;
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos) std::cout << "=";
    else if (i == pos) std::cout << ">";
    else std::cout << " ";
  }

  // Schedule the next update if the simulation isn't finished
  if (progress < 1.0) {
    std::cout << "] " << std::fixed << std::setprecision(1) << (progress * 100.0) << " %" << std::flush;

    // Update every 100ms of simulation time
    Simulator::Schedule(MilliSeconds(100), &ShowProgress, start, stop);
  }
}

void
InstallApplication(Ptr<Node> node, std::shared_ptr<caf::ZoR> zor, Time startTime, Time stopTime, double threshold)
{
  // static variables are initialized only once
  static ndn::AppHelper producerHelper("ns3::ndn::RandomAlertProducer");
  producerHelper.SetAttribute("StartTime", TimeValue(startTime));
  producerHelper.SetAttribute("StopTime", TimeValue(stopTime));
  producerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/veh/" + std::to_string(node->GetId())));
  producerHelper.SetAttribute("Interval", StringValue("0s")); // only once
  producerHelper.SetAttribute("PayloadSize", UintegerValue(64));
  producerHelper.SetAttribute("Threshold", DoubleValue(threshold));

  auto app = DynamicCast<ndn::RandomAlertProducer>(producerHelper.Install(node).Get(0));
  app->SetZoR(zor);
}

int
main (int argc, char *argv[])
{
  const double txRadius = 50.0;
  const double roadWidth = 21.0;
  const double maxRange = 400.0; // extend 400m in all directions

  // ------------------------------------------------------------
  // Command-line parameter
  // ------------------------------------------------------------
  double startTime = 60.0;
  double stopTime = 120.0;
  std::string mode = "low";
  bool enableHello = false;
  bool disableRsu = false;
  double threshold = 0.5;

  CommandLine cmd;
  cmd.AddValue("startTime", "Start time (default: 60s)", startTime);
  cmd.AddValue("stopTime", "Stop time (default: 90s)", stopTime);
  cmd.AddValue("mode", "Vehicle Density Mode (low|medium|high) (default: low)", mode);
  cmd.AddValue("enableHello", "Whether to enable hello messages or not (default: false)", enableHello);
  cmd.AddValue("disableRsu", "Whether to disable RSU or not (default: false)", disableRsu);
  cmd.AddValue("threshold", "Probability threshold for alert producer (default: 0.5)", threshold);
  cmd.Parse (argc, argv);

  std::cout << "CLI arguments:" << '\n' 
            << "| startTime="<<startTime << '\n'
            << "| stopTime="<<stopTime << '\n'
            << "| mode="<<mode << '\n'
            << "| enableHello="<<enableHello << '\n'
            << "| disableRsu="<<disableRsu << '\n'
            << "| threshold="<<threshold << std::endl;

  // ------------------------------------------------------------
  // Build animation filename
  // ------------------------------------------------------------
  std::string traceFile = "trace/scene-" + mode + ".tcl";
  std::string animFile = "netanim/scene-" + mode + ".xml";

  // ------------------------------------------------------------
  // Generate RSU positions
  // ------------------------------------------------------------
  std::vector<Vector> rsuPositions;

  // Centre node
  rsuPositions.emplace_back (Vector (0.0, 0.0, 0.0));

  double dx = h (txRadius, roadWidth);
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
    {10, 200},
    {10, -200},
    {-10, -200},
    {-10, 200},
  }));
  zor->append(std::make_unique<caf::PolygonZoR>(std::vector<caf::Point>{
    {200, 10},
    {200, -10},
    {-200, -10},
    {-200, 10},
  }));
  
  auto rand = CreateObject<UniformRandomVariable>();

  for (const auto& it: vehicleLifetime) {
    auto nodeId = it.first;
    auto inTime = it.second.first + 30 + rand->GetValue(); // +30seconds buffer time
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

    InstallApplication(node, zor, Seconds(inTime), Seconds(outTime), threshold);
    std::cout << "Install application on vehicle("<< node->GetId() <<") at="<<inTime<<"s" << std::endl;
  }

  // ------------------------------------------------------------
  // NetAnim
  // ------------------------------------------------------------
  // AnimationInterface anim (animFile);
  // if (enableHello) {
  //   setNodesColor(anim, rsuNodes, 0, 255, 0); // default: green (active)
  // } else {
  //   setNodesColor(anim, rsuNodes, 255, 0, 0); // red (inactive)
  // }
  // if (disableRsu) {
  //   setNodesColor(anim, rsuNodes.Get(0), 255, 0, 0); // red (inactive)
  // }
  // setNodesColor(anim, vehicleNodes, 0, 0, 255); // default: blue

  Simulator::Stop (Seconds (stopTime));
  std::cout << "Simulator duration set to " << stopTime << " seconds" << std::endl;
  Simulator::Schedule(Seconds(startTime), &ShowProgress, startTime, stopTime);
  Simulator::Run ();
  Simulator::Destroy ();
  
  std::cout << '\n'
            << "Simulation completed" << std::endl;

  return 0;
}

} // namespace ns3

int
main (int argc, char *argv[])
{
  return ns3::main (argc, argv);
}
