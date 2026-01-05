#ifndef SCRATCH_UTILS_H
#define SCRATCH_UTILS_H

#include "ns3/node-container.h"
#include "ns3/string.h"
#include "ns3/wifi-module.h"
#include "ns3/config.h"

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
SetDefaultP2PConfig()
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", StringValue ("20p"));
}

}

#endif