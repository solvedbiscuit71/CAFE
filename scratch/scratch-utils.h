#ifndef SCRATCH_UTILS_H
#define SCRATCH_UTILS_H

#include "ns3/constant-position-mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/string.h"
#include "ns3/wifi-module.h"
#include "ns3/config.h"
#include "ns3/netanim-module.h"

namespace ns3 {

inline void
SetupWifiNetDevice (const NodeContainer &nodes)
{
  // 1. Setup Wireless Channel and Physical Layer
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (wifiChannel.Create ());

  // 2. Setup WiFi MAC and Standard
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211p);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate9Mbps"), "ControlMode",
                                StringValue ("OfdmRate9Mbps"));

  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");

  // 3. Install WiFi on Nodes
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);
}

inline void
SetDefaultP2PConfig ()
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", StringValue ("20p"));
}

using NodeLifetime = std::map<uint32_t, std::pair<double, double>>;

inline void
ParseMobilityTrace (const std::string &traceFile, uint32_t &numNodes, NodeLifetime &nodeLifetime,
                    double &duration)
{
  double time;
  uint32_t nodeId;
  std::ifstream file (traceFile);
  std::string line;

  std::map<uint32_t, double> firstSeen;
  std::map<uint32_t, double> lastSeen;

  while (std::getline (file, line))
    {
      if (line.find ("$node_(") != std::string::npos)
        {

          if (sscanf (line.c_str (), "$ns_ at %lf \"$node_(%u)", &time, &nodeId) == 2)
            {
              if (firstSeen.find (nodeId) == firstSeen.end ())
                {
                  firstSeen[nodeId] = time;
                }
              lastSeen[nodeId] = time;
            }
        }
    }

  for (auto const &[nodeId, stopTime] : lastSeen)
    {
      nodeLifetime[nodeId] = {firstSeen[nodeId], stopTime};
    }

  duration = time;
  numNodes = nodeLifetime.size ();

  std::cout << "Parsed mobility trace.\n"
            << "Found " << numNodes << " nodes.\n"
            << "Simulation duration set to " << duration << " seconds." << std::endl;
}

inline NodeContainer
createNodeWith (uint32_t numMobilityNodes, std::string traceFile)
{
  NodeContainer nodes;
  nodes.Create (numMobilityNodes);

  // Create Ns2MobilityHelper with the specified trace log file as parameter
  Ns2MobilityHelper mobilityHelper = Ns2MobilityHelper (traceFile);
  mobilityHelper.Install (nodes.Begin (), nodes.End ());
  return nodes;
}

inline NodeContainer
createNodeAt (uint32_t numRSUNodes, std::vector<Vector> &positions)
{
  NodeContainer nodes;
  for (const auto &pos : positions)
    {
      // Create a new node for the RSU
      Ptr<Node> node = CreateObject<Node> ();
      nodes.Add (node);

      // Install a static mobility model on the RSU node
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
      if (!mobility)
        {
          // If the node doesn't have a mobility model, add one
          Ptr<ConstantPositionMobilityModel> positionModel =
              CreateObject<ConstantPositionMobilityModel> ();
          node->AggregateObject (positionModel);
          mobility = positionModel;
        }

      // Set the position of the RSU
      mobility->SetPosition (pos);
    }
  return nodes;
}

inline void
setNodesColor (AnimationInterface &anim, const NodeContainer &nodes, uint8_t red, uint8_t green,
               uint8_t blue)
{
  for (uint32_t i = 0; i < nodes.GetN (); ++i)
    {
      uint32_t nodeId = nodes.Get (i)->GetId ();
      anim.UpdateNodeColor (nodeId, red, green, blue);
    }
}

inline void
setNodeColor (AnimationInterface &anim, Ptr<Node> node, uint8_t red, uint8_t green, uint8_t blue)
{
  uint32_t nodeId = node->GetId ();
  anim.UpdateNodeColor (nodeId, red, green, blue);
}

} // namespace ns3

#endif