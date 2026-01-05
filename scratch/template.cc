#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-module.h"
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
  // NodeContainer nodes;
  // nodes.Create (numOfNodes)

  // * Install Network Stack

  // * Install Application

  // * Enable NetAnim
  // AnimationInterface anim ("netanim/template.xml");

  Simulator::Stop (Seconds (0.0));
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