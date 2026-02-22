#include "apps/random-alert-producer.hpp"
#include "helper/ndn-app-helper.hpp"
#include "helper/ndn-global-routing-helper.hpp"
#include "helper/caf-stack-helper.hpp"
#include "helper/ndn-strategy-choice-helper.hpp"
#include "model/caf-context.hpp"
#include "model/caf-zor.hpp"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"
#include "ns3/vector.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"

#include "scratch-utils.h"
#include <cstdint>
#include <memory>

namespace ns3 {

double f(double r, double w, double delta) {
  return (2.0 * std::sqrt(r * r - w * w)) + delta;
}

double g(double r, double w, double delta) {
  return (std::sqrt(2.0 * r * r - w * w + r * f(r, w, delta))) + delta;
}

double h(double r, double w, double delta) {
  return (std::sqrt(4.0 * r * r - w * w)) + delta;
}

int
main (int argc, char *argv[])
{
  // * Read optional command-line parameters
  // CommandLine cmd;
  // cmd.Parse (argc, argv);

  // * Creating nodes
  NodeContainer rsu;
  rsu.Create(7);
  
  caf::setupContext(rsu, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_RSU);
    ctx->SetNodeStatus(caf::NODE_STATUS_ACTIVE);
  });
  
  // use middle placement strategy
  double dx = g(50.0, 3.5, 0.0);
  std::cout << "RSU placed " << dx << "m apart." << std::endl;
  MobilityHelper rsuMobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (  0,   0,   0)); // RSU 0
  positionAlloc->Add (Vector (  0,  dx,   0)); // RSU 1
  positionAlloc->Add (Vector ( dx,   0,   0)); // RSU 2
  positionAlloc->Add (Vector (  0, -dx,   0)); // RSU 3
  positionAlloc->Add (Vector ( dx, -dx,   0)); // RSU 4
  positionAlloc->Add (Vector ( dx,  dx,   0)); // RSU 5
  positionAlloc->Add (Vector (-dx,  dx,   0)); // RSU 6

  rsuMobility.SetPositionAllocator (positionAlloc);
  rsuMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rsuMobility.Install (rsu);
  
  SetDefaultP2PConfig();
  PointToPointHelper p2p;
  p2p.Install(rsu.Get(0), rsu.Get(1));
  p2p.Install(rsu.Get(0), rsu.Get(2));
  p2p.Install(rsu.Get(0), rsu.Get(3));
  p2p.Install(rsu.Get(1), rsu.Get(5));
  p2p.Install(rsu.Get(1), rsu.Get(6));
  p2p.Install(rsu.Get(2), rsu.Get(4));
  p2p.Install(rsu.Get(2), rsu.Get(5));
  p2p.Install(rsu.Get(3), rsu.Get(4));
  
  NodeContainer adhocNodes;
  adhocNodes.Add(rsu);
  SetupWifiNetDevice(adhocNodes);

  caf::StackHelper stackHelper;
  stackHelper.setEnableHello(false); // enable hello producer and consumer
  stackHelper.Install(rsu);

  // * Install consumer
  ndn::AppHelper consumerHelper("ns3::ndn::AlertConsumer");
  consumerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/rsu"));
  consumerHelper.SetAttribute("LifeTime", StringValue("30s"));
  
  auto consumerApps = consumerHelper.Install(rsu);
  consumerApps.Start(Seconds(1.0));
  
  // * Build ZoR
  auto zor = make_shared<caf::CompositeZoR>();
  float dxx = static_cast<float>(dx);

  zor->append(std::make_unique<caf::PolygonZoR>(std::vector<caf::Point>{
    {5,dxx+5},
    {5,dxx-5},
    {-dxx-5,dxx+5},
    {-dxx-5,dxx-5},
  }));
  zor->append(std::make_unique<caf::PolygonZoR>(std::vector<caf::Point>{
    {dxx+5,5},
    {dxx-5,5},
    {dxx+5,-dxx-5},
    {dxx-5,-dxx-5},
  }));

  ndn::AppHelper producerHelper("ns3::ndn::RandomAlertProducer");
  producerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/rsu/" + std::to_string(rsu.Get(0)->GetId())));
  producerHelper.SetAttribute("Interval", StringValue("1s"));
  producerHelper.SetAttribute("PayloadSize", UintegerValue(64));
  producerHelper.SetAttribute("Threshold", DoubleValue(0.5)); // 50% chance
  
  auto producerApps = producerHelper.Install(rsu.Get(0));
  producerApps.Start(Seconds(1.0));
  auto app = DynamicCast<ndn::RandomAlertProducer>(producerApps.Get(0));
  app->SetZoR(zor);
  
  // * Enable NetAnim
  AnimationInterface anim ("netanim/mira-sim.xml");

  Simulator::Stop (Seconds (5.0));
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