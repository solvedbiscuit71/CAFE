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
  // * Read optional command-line parameters
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // * Creating nodes
  NodeContainer vehicle;
  vehicle.Create(15);
  caf::setupContext(vehicle, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_VEHICLE);
    ctx->SetNodeStatus(caf::NODE_STATUS_ACTIVE);
  });

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
  NodeContainer adhocNodes;
  adhocNodes.Add(vehicle);
  SetupWifiNetDevice(adhocNodes);

  caf::StackHelper stackHelper;
  stackHelper.setEnableHello(false); // enable hello producer and consumer
  stackHelper.Install(vehicle, true);

  // * Install consumer
  ndn::AppHelper consumerHelper("ns3::ndn::AlertConsumer");
  consumerHelper.SetAttribute("StartTime", StringValue("1s"));
  consumerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/veh"));
  consumerHelper.SetAttribute("LifeTime", StringValue("30s"));
  
  auto consumerApps = consumerHelper.Install(vehicle);

  // * Build ZoR
  auto zor = std::make_shared<caf::PolygonZoR>(std::vector<caf::Point>{
    {100,10},
    {200,10},
    {200,-10},
    {100,-10},
  });

  ndn::AppHelper producerHelper("ns3::ndn::RandomAlertProducer");
  consumerHelper.SetAttribute("StartTime", StringValue("1s"));
  producerHelper.SetAttribute("Prefix", StringValue("/alert/emergency/veh/" + std::to_string(vehicle.Get(0)->GetId())));
  producerHelper.SetAttribute("Interval", StringValue("0s"));
  producerHelper.SetAttribute("PayloadSize", UintegerValue(64));
  producerHelper.SetAttribute("Threshold", DoubleValue(1.0)); // 100% chance
  
  auto producerApps = producerHelper.Install(vehicle.Get(0));
  auto app = DynamicCast<ndn::RandomAlertProducer>(producerApps.Get(0));
  app->SetZoR(zor);

  // * Enable NetAnim
  AnimationInterface anim ("netanim/geo-sim.xml");
  
  Simulator::Schedule(Seconds(1.0), &countInside, zor);

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