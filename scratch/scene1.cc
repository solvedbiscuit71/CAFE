#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"

#include "scratch-utils.h"

#include <cmath>
#include <vector>
#include <string>

namespace ns3 {

  // ----------------------------------------------------------------
  // Distance Calculation Formula
  // ----------------------------------------------------------------

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
  const double RADIUS   = 50.0;    // transmission radius (m)
  const double ROAD_LEN = 1000.0;  // 1 km road
  const double roadWidth = 21.0;

  std::string traceFile = "trace/scene1.tcl";
  std::string animFile  = "netanim/scene1.xml";

  // ----------------------------------------------------------------
  // Command-line parameters
  // ----------------------------------------------------------------
  std::string placement = "one-side";

  double delta = 1.0;

  CommandLine cmd;
  cmd.AddValue("placement", "RSU placement: one-side | both-side | middle", placement);
  cmd.AddValue("delta", "buffer in meters", delta);
  cmd.Parse(argc, argv);

  // ----------------------------------------------------------------
  // Select spacing function
  // ----------------------------------------------------------------
  double dx;

  if (placement == "one-side") {
    dx = f(RADIUS, roadWidth, delta);
  }
  else if (placement == "both-side") {
    dx = g(RADIUS, roadWidth, delta);
  }
  else if (placement == "middle") {
    dx = h(RADIUS, roadWidth, delta);
  }
  else {
    NS_FATAL_ERROR("Invalid placement strategy: " << placement);
  }

  // ----------------------------------------------------------------
  // Generate RSU positions from (0,0,0) to 1 km
  // ----------------------------------------------------------------
  std::vector<Vector> rsuPositions;

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  while (x <= ROAD_LEN) {
    rsuPositions.emplace_back(Vector(x, y, z));
    x += dx;
  }

  uint32_t numRsu = rsuPositions.size();
  NodeContainer rsuNodes = createNodeAt(numRsu, rsuPositions);

  // ----------------------------------------------------------------
  // Mobility (vehicles)
  // ----------------------------------------------------------------
  uint32_t numVehicles;
  double duration;
  NodeLifetime vehicleLifetime;

  ParseMobilityTrace(traceFile,
                     numVehicles,
                     vehicleLifetime,
                     duration);

  NodeContainer vehicleNodes = createNodeWith(numVehicles, traceFile);

  // ----------------------------------------------------------------
  // NetAnim
  // ----------------------------------------------------------------
  AnimationInterface anim(animFile);
  setNodesColor(anim, vehicleNodes, 255, 0, 0); // vehicles: red
  setNodesColor(anim, rsuNodes, 0, 0, 255);     // RSUs: blue

  Simulator::Stop(Seconds(duration));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main (int argc, char *argv[])
{
  return ns3::main(argc, argv);
}
