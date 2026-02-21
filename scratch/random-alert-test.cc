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
#include "apps/random-alert-producer.hpp"

#include "scratch-utils.h"
#include <cstdint>
#include <memory>

namespace ns3 {

std::shared_ptr<caf::ZoR>
sampleZoR ()
{
  using namespace ns3::caf;
  auto zor = make_shared<CompositeZoR>();

  std::unique_ptr<CompositeZoR> subZoR = std::make_unique<CompositeZoR> ();
  subZoR->append (std::make_unique<PolygonZoR> (
      std::vector<Point>{{200, 5}, {200, -5}, {-200, -5}, {-200, 5}}));
  subZoR->append (std::make_unique<PolygonZoR> (
      std::vector<Point>{{5, 200}, {5, -200}, {-5, -200}, {-5, 200}}));

  zor->append (std::make_unique<CircleZoR> (Point{0, 0}, 50));
  zor->append (std::move (subZoR));


  return zor;
}

int
main (int argc, char *argv[])
{
  // * Read command-line parameters
  // Must when using the --RngRun=<number> flag
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer vehicle;
  vehicle.Create(1);
  caf::setupContext(vehicle, [](Ptr<caf::Context> ctx) {
    ctx->SetNodeType(caf::NODE_TYPE_VEHICLE);
    ctx->SetNodeStatus(caf::NODE_STATUS_ACTIVE);
  });

  Ptr<ConstantVelocityMobilityModel> vehicle0Mobility = CreateObject<ConstantVelocityMobilityModel>();
  vehicle0Mobility->SetPosition(Vector (0.0, 0.0, 0.0));
  vehicle0Mobility->SetVelocity(Vector(24.0, 0.0, 0.0));
  vehicle.Get(0)->AggregateObject(vehicle0Mobility);

  NodeContainer adhocNodes;
  adhocNodes.Add(vehicle);
  SetupWifiNetDevice(adhocNodes);

  caf::StackHelper stackHelper;
  stackHelper.setEnableHello(false); // enable hello producer and consumer
  stackHelper.Install(vehicle, true);

  /**
   * To model real world scenario, we need to add randomness to the equation.
   */
  Ptr<NormalRandomVariable> rng = CreateObject<NormalRandomVariable>();
  rng->SetAttribute("Mean", DoubleValue(0.0));
  rng->SetAttribute("Variance", DoubleValue(0.0001)); // std = 10ms; var = 0.01 ^ 2 = 0.0001
  
  std::cout << "rng->GetValue(): " << rng->GetValue() << std::endl;

  ndn::AppHelper alertProducer("ns3::ndn::RandomAlertProducer");
  alertProducer.SetAttribute("Prefix", StringValue("/alert/emergency/veh/" + std::to_string(vehicle.Get(0)->GetId())));
  alertProducer.SetAttribute("Interval", StringValue("1s"));
  alertProducer.SetAttribute("PayloadSize", UintegerValue(64));
  alertProducer.SetAttribute("Threshold", DoubleValue(0.5)); // 50% chance
  
  auto apps = alertProducer.Install(vehicle.Get(0));

  auto testZoR = sampleZoR();

  // start the producer app at random seconds
  for (uint32_t i=0; i<apps.GetN(); i++) {
    Ptr<ndn::RandomAlertProducer> app = DynamicCast<ndn::RandomAlertProducer>(apps.Get(i));
    if (app) {
      app->SetStartTime(Seconds(1.0 + rng->GetValue()));
      app->SetZoR(testZoR);
    }
  }
  apps.Stop(Seconds(10.0)); // stop the consumer app at 10 seconds mark

  // * Enable NetAnim
  AnimationInterface anim ("netanim/random-alert-test.xml");

  Simulator::Stop (Seconds (10.0));
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