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
#include <sstream>
#include <iomanip>

namespace ns3 {

// ------------------------------------------------------------
// Middle placement spacing function
// ------------------------------------------------------------
double h(double r, double w, double delta) {
  return std::sqrt(4.0 * r * r - w * w) + delta;
}

int
main (int argc, char *argv[])
{
  const double txRadius   = 50.0;
  const double roadWidth  = 21.0;
  const double maxRange   = 400.0;  // extend 400m in all directions

  std::string traceFile = "trace/scene2.tcl";

  // ------------------------------------------------------------
  // Command-line parameter
  // ------------------------------------------------------------
  double delta = 0.0;

  CommandLine cmd;
  cmd.AddValue("delta", "Deviation from optimal spacing", delta);
  cmd.Parse(argc, argv);

  // ------------------------------------------------------------
  // Build animation filename
  // ------------------------------------------------------------
  std::ostringstream oss;
  oss << "netanim/scene2-"
      << std::fixed << std::setprecision(1)
      << delta << ".xml";

  std::string animFile = oss.str();


// ------------------------------------------------------------
// Generate RSU positions
// ------------------------------------------------------------
    std::vector<Vector> rsuPositions;

    // Centre node
    rsuPositions.emplace_back(Vector(0.0, 0.0, 0.0));

    double dx = h(txRadius, roadWidth, delta);
    int maxLayer = static_cast<int>(maxRange / dx);

    // +X direction
    for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back(Vector(i * dx, 0.0, 0.0));
    }

    // -X direction
    for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back(Vector(-i * dx, 0.0, 0.0));
    }

    // +Y direction
    for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back(Vector(0.0, i * dx, 0.0));
    }

    // -Y direction
    for (int i = 1; i <= maxLayer; ++i) {
    rsuPositions.emplace_back(Vector(0.0, -i * dx, 0.0));
    }


  uint32_t numRsu = rsuPositions.size();
  NodeContainer rsuNodes = createNodeAt(numRsu, rsuPositions);

  for (uint32_t i = 0; i < rsuPositions.size(); ++i) {
  std::cout << "RSU ID " << i
            << " -> (" << rsuPositions[i].x
            << ", " << rsuPositions[i].y << ")\n";
}


  // ------------------------------------------------------------
  // Mobility (vehicles)
  // ------------------------------------------------------------
  uint32_t numVehicles;
  double duration;
  NodeLifetime vehicleLifetime;

  ParseMobilityTrace(traceFile,
                     numVehicles,
                     vehicleLifetime,
                     duration);

  NodeContainer vehicleNodes = createNodeWith(numVehicles, traceFile);
  

  std::cout << "Duration: " << duration << std::endl;
std::cout << "RSU count: " << numRsu << std::endl;


  // ------------------------------------------------------------
  // NetAnim
  // ------------------------------------------------------------
  AnimationInterface anim(animFile);
  setNodesColor(anim, vehicleNodes, 255, 0, 0);
  setNodesColor(anim, rsuNodes, 0, 0, 255);

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
