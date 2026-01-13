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

  // * Options
  // Input stream
  // TODO: **** Change the filename ****
  std::string traceFile = "trace/template.tcl";

  // Output stream (ignored by git)
  // TODO: **** Change the filename ****
  std::string animFile = "netanim/template.xml";

  // * Creating nodes
  // Create RSU nodes
  std::vector<Vector> rsuPositions;
  // TODO: **** Change RSU position ****
  rsuPositions.push_back(Vector(0.0, 0.0, 0.0));
  uint32_t numRsu = rsuPositions.size();

  NodeContainer rsuNodes = createNodeAt(numRsu, rsuPositions);

  // Create Mobility nodes
  uint32_t numVehicles;
  double   duration;
  NodeLifetime vehicleLifetime;

  ParseMobilityTrace(traceFile,
                     numVehicles,
                     vehicleLifetime,
                     duration);
  NodeContainer vehicleNodes = createNodeWith(numVehicles, traceFile);
  
  // * Install Network Stack

  // * Install Application

  // * Enable NetAnim
  AnimationInterface anim (animFile);

  Simulator::Stop (Seconds (duration));
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