#include "helper/ndn-app-helper.hpp"
#include "helper/ndn-global-routing-helper.hpp"
#include "helper/caf-stack-helper.hpp"
#include "helper/ndn-strategy-choice-helper.hpp"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/string.h"
#include "ns3/vector.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"

#include "scratch-utils.h"

namespace ns3 {

double f(double r, double w, double delta) {
  return (2.0 * std::sqrt(r * r - w * w)) + delta;
}

double g(double r, double w, double delta) {
  return (std::sqrt(2.0 * r * r - w * w + r * f(r, w, delta))) + delta;
}

/*
 * Vehilce#0 should sent interest to multicast_v2v
 * Vehilce#1 shoudl sent data back however RSU#0 should drop the packet at transport
 */
int
main (int argc, char *argv[])
{

  // * Read optional command-line parameters
  // CommandLine cmd;
  // cmd.Parse (argc, argv);

  // * Creating nodes
  NodeContainer rsu;
  rsu.Create(2);
  caf::setupContext(rsu, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_RSU);
  });
  
  NodeContainer vehicle;
  vehicle.Create(2);
  caf::setupContext(vehicle, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_VEHICLE);
  });

  double dx = g(50.0, 3.5, 0.0);
  std::cout << "RSU placed " << dx << "m apart." << std::endl;
  MobilityHelper rsuMobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, -1.75, 0.0));
  positionAlloc->Add (Vector (dx, 1.75, 0.0));

  rsuMobility.SetPositionAllocator (positionAlloc);
  rsuMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rsuMobility.Install (rsu);
  
  Ptr<ConstantVelocityMobilityModel> vehicle0Mobility = CreateObject<ConstantVelocityMobilityModel>();
  vehicle0Mobility->SetPosition(Vector (0.0, 0.0, 0.0));
  vehicle0Mobility->SetVelocity(Vector (24.0, 0.0, 0.0));
  vehicle.Get(0)->AggregateObject(vehicle0Mobility);

  Ptr<ConstantVelocityMobilityModel> vehicle1Mobility = CreateObject<ConstantVelocityMobilityModel>();
  vehicle1Mobility->SetPosition(Vector (20.0, 0.0, 0.0));
  vehicle1Mobility->SetVelocity(Vector (24.0, 0.0, 0.0));
  vehicle.Get(1)->AggregateObject(vehicle1Mobility);

  // * Install Network Stack
  SetDefaultP2PConfig();
  PointToPointHelper p2p;
  p2p.Install(rsu.Get(0), rsu.Get(1));
  
  NodeContainer adhocNodes;
  adhocNodes.Add(vehicle);
  adhocNodes.Add(rsu);
  SetupWifiNetDevice(adhocNodes);

  caf::StackHelper stackHelper;
  stackHelper.Install(rsu, true);
  stackHelper.Install(vehicle, true);

  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");

  // * Install Application
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("1"));
  consumerHelper.Install(vehicle.Get(0));

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(rsu);

  // * Enable NetAnim
  AnimationInterface anim ("netanim/test-transport.xml");

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