// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include "ns3/netanim-module.h"

#include "scratch-utils.h"

namespace ns3 {

void
MobilitySetup(const NodeContainer& nodes) {
  MobilityHelper mobility;

  // 1. Create a list of positions
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0)); // Node 0 position
  positionAlloc->Add (Vector (40.0, 0.0, 0.0)); // Node 1 position (50m away)
  positionAlloc->Add (Vector (80.0, 0.0, 0.0)); // Node 2 position (100m away)

  // 2. Tell the helper to use this list
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
} 

int
main (int argc, char *argv[])
{
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create (3);

  // Install NetDevice and Mobility
  SetupWifiNetDevice(nodes);
  MobilitySetup(nodes);
  
  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.SetWifiAsAdhoc(true);
  ndnHelper.InstallAll ();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll ("/prefix", "/localhost/nfd/strategy/multicast");

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix ("/prefix");
  consumerHelper.SetAttribute ("Frequency", StringValue ("1")); // 10 interests a second
  auto apps = consumerHelper.Install (nodes.Get (0)); // first node
  apps.Stop (Seconds (1.0)); // stop the consumer app at 10 seconds mark

  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix");
  producerHelper.SetAttribute ("PayloadSize", StringValue ("1024"));
  producerHelper.Install (nodes.Get (2)); // last node

  Simulator::Stop (Seconds (5.0));

  // NetAnim
  AnimationInterface anim("netanim/ndn-simple.xml");

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