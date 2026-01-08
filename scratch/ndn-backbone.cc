#include "helper/ndn-app-helper.hpp"
#include "helper/ndn-global-routing-helper.hpp"
#include "helper/ndn-stack-helper.hpp"
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

int
main (int argc, char *argv[])
{

  // * Read optional command-line parameters
  // CommandLine cmd;
  // cmd.Parse (argc, argv);

  // * Creating nodes
  NodeContainer rsu;
  rsu.Create(3);
  
  NodeContainer backBone;
  backBone.Create(1);

  NodeContainer vehicle;
  vehicle.Create(1);

  MobilityHelper rsuMobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (80.0, 0.0, 0.0));
  positionAlloc->Add (Vector (160.0, 0.0, 0.0));

  rsuMobility.SetPositionAllocator (positionAlloc);
  rsuMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rsuMobility.Install (rsu);
  
  Ptr<ConstantVelocityMobilityModel> vehicle0Mobility = CreateObject<ConstantVelocityMobilityModel>();
  vehicle0Mobility->SetPosition(Vector (0.0, 0.0, 0.0));
  vehicle0Mobility->SetVelocity(Vector (24.0, 0.0, 0.0));
  vehicle.Get(0)->AggregateObject(vehicle0Mobility);

  // * Install Network Stack
  SetDefaultP2PConfig();
  PointToPointHelper p2p;
  p2p.Install(rsu.Get(0), rsu.Get(1));
  p2p.Install(rsu.Get(1), rsu.Get(2));
  p2p.Install(rsu.Get(0), backBone.Get(0));
  p2p.Install(rsu.Get(1), backBone.Get(0));
  p2p.Install(rsu.Get(2), backBone.Get(0));
  
  NodeContainer adhocNodes;
  adhocNodes.Add(vehicle);
  adhocNodes.Add(rsu);
  SetupWifiNetDevice(adhocNodes);

  ndn::StackHelper rsuHelper;
  rsuHelper.SetNodeType(NODE_TYPE_RSU);
  rsuHelper.Install(rsu);

  ndn::StackHelper vehicleHelper;
  vehicleHelper.SetDefaultRoutes(true);
  vehicleHelper.SetNodeType(NODE_TYPE_VEHICLE);
  vehicleHelper.Install(vehicle);

  ndn::StackHelper backBoneHelper;
  backBoneHelper.SetNodeType(NODE_TYPE_BACKBONE);
  backBoneHelper.Install(backBone);
  
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  
  NodeContainer routableNodes;
  routableNodes.Add(rsu);
  routableNodes.Add(backBone);
  ndn::GlobalRoutingHelper ndnRoutingHelper;
  ndnRoutingHelper.Install(routableNodes);

  // * Install Application
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("1"));
  consumerHelper.Install(vehicle.Get(0));
  
  
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(backBone.Get(0));
  
  ndnRoutingHelper.AddOrigin("/prefix", backBone.Get(0));
  ndnRoutingHelper.CalculateAllPossibleRoutes();

  // * Enable NetAnim
  AnimationInterface anim ("netanim/ndn-backbone.xml");

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