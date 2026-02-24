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
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
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

double h(double r, double w, double delta) {
  return (std::sqrt(4.0 * r * r - w * w)) + delta;
}

void
countInside(shared_ptr<caf::ZoR> zor) 
{
  int count = 0;
  for (uint32_t i = 0; i < NodeList::GetNNodes(); ++i) {
    Ptr<Node> node = NodeList::GetNode(i);
    
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    if (mobility) {
        Vector pos = mobility->GetPosition();
        if (zor->contains(caf::Point{static_cast<float>(pos.x), static_cast<float>(pos.y)})) {
          count += 1;
        }
    }
  }
  std::cout << "At t=1.0s ZoR has " << count << " nodes inside" << std::endl;
}

int
main (int argc, char *argv[])
{
  bool enableHello = false;
  bool disableRsu = false;

  // * Read optional command-line parameters
  CommandLine cmd;
  cmd.AddValue("enableHello", "Whether to enable hello messages or not", enableHello);
  cmd.AddValue("disableRsu", "Whether to disable RSU or not", disableRsu);
  cmd.Parse (argc, argv);

  std::cout << "enableHello="<<enableHello<<" disableRsu="<<disableRsu<<"" << std::endl;

  // * Creating nodes
  NodeContainer rsu;
  rsu.Create(5);

  caf::setupContext(rsu, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_RSU);
    ctx->SetNodeStatus(caf::NODE_STATUS_ACTIVE);
  });

  NodeContainer vehicle;
  vehicle.Create(20);
  caf::setupContext(vehicle, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_VEHICLE);
    ctx->SetNodeStatus(caf::NODE_STATUS_ACTIVE);
  });

  //@ assume RSU#3 is under maintainence
  if (disableRsu) {
    rsu.Get(3)->GetObject<caf::Context>()->SetNodeStatus(caf::NODE_STATUS_INACTIVE);
  }

  // use middle placement strategy
  double dx = h(50.0, 10, 0.0);
  std::cout << "RSU placed " << dx << "m apart." << std::endl;
  MobilityHelper rsuMobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint32_t i=0; i<rsu.GetN(); i++) {
    positionAlloc->Add (Vector (i * dx, 0.0, 0.0));
  }
  rsuMobility.SetPositionAllocator (positionAlloc);
  rsuMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rsuMobility.Install (rsu);

  /**
   * To model real world scenario, we need to add randomness to the equation.
   */
  Ptr<NormalRandomVariable> rng = CreateObject<NormalRandomVariable>();
  rng->SetAttribute("Mean", DoubleValue(0.0));
  rng->SetAttribute("Variance", DoubleValue(10)); // std = 5; var = 5^2 = 25

  for (uint32_t i=0; i<vehicle.GetN(); i++) {
    auto m = CreateObject<ConstantVelocityMobilityModel>();
    m->SetPosition(Vector (20.0 * i + rng->GetValue(), rng->GetValue(), 0.0));
    m->SetVelocity(Vector(15.0 + rng->GetValue(), 0.0, 0.0));
    vehicle.Get(i)->AggregateObject(m);
  }

  // * Install Network Stack
  SetDefaultP2PConfig();
  PointToPointHelper p2p;
  p2p.Install(rsu.Get(0), rsu.Get(1));
  p2p.Install(rsu.Get(1), rsu.Get(2));
  if (!disableRsu) {
    p2p.Install(rsu.Get(2), rsu.Get(3));
    p2p.Install(rsu.Get(3), rsu.Get(4));
  }

  NodeContainer adhocNodes;
  adhocNodes.Add(rsu);
  adhocNodes.Add(vehicle);
  SetupWifiNetDevice(adhocNodes);

  caf::StackHelper stackHelper;
  stackHelper.setEnableHello(enableHello); // enable hello producer and consumer
  stackHelper.Install(rsu);
  stackHelper.Install(vehicle, true);
  

  // * Install consumer
  ndn::AppHelper consumerHelper("ns3::ndn::AlertConsumer");
  consumerHelper.SetAttribute("StartTime", StringValue("1s"));
  consumerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/veh"));
  consumerHelper.SetAttribute("LifeTime", StringValue("30s"));
  
  auto consumerApps = consumerHelper.Install(vehicle);

  // * Build ZoR
  auto zor = std::make_shared<caf::PolygonZoR>(std::vector<caf::Point>{
    {300,10},
    {400,10},
    {400,-10},
    {300,-10},
  });

  ndn::AppHelper producerHelper("ns3::ndn::RandomAlertProducer");
  producerHelper.SetAttribute("StartTime", StringValue("1s"));
  producerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/veh/" + std::to_string(vehicle.Get(0)->GetId())));
  producerHelper.SetAttribute("Interval", StringValue("0s"));
  producerHelper.SetAttribute("PayloadSize", UintegerValue(64));
  producerHelper.SetAttribute("Threshold", DoubleValue(1.0)); // 100% chance
  
  auto producerApps = producerHelper.Install(vehicle.Get(0));
  auto app = DynamicCast<ndn::RandomAlertProducer>(producerApps.Get(0));
  app->SetZoR(zor);

  // * Enable NetAnim
  AnimationInterface anim ("netanim/healing-sim.xml");
  if (enableHello) {
    setNodesColor(anim, rsu, 0, 255, 0); // default: green (active)
  } else {
    setNodesColor(anim, rsu, 255, 0, 0); // red (inactive)
  }
  if (disableRsu) {
    setNodesColor(anim, rsu.Get(3), 255, 0, 0); // red (inactive)
  }
  setNodesColor(anim, vehicle, 0, 0, 255); // default: blue
  
  Simulator::Schedule(Seconds(2.0), &countInside, zor);

  Simulator::Stop (Seconds (3.0));
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