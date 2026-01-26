/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2018  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndn-wifi-net-device-transport.hpp"

#include "../helper/ndn-stack-helper.hpp"
#include "model/caf-context.hpp"
#include "ndn-block-header.hpp"
#include "../utils/ndn-ns3-packet-tag.hpp"

#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>

#include "ns3/address.h"
#include "ns3/ipv6-address.h"
#include "ns3/queue.h"

NS_LOG_COMPONENT_DEFINE("ndn.WifiNetDeviceTransport");

namespace ns3 {
namespace ndn {

WifiNetDeviceTransport::WifiNetDeviceTransport(Ptr<Node> node,
                                       const Ptr<NetDevice>& netDevice,
                                       const Address& remoteAddress,
                                       caf::TransportFilter filter,
                                       ::ndn::nfd::FaceScope scope,
                                       ::ndn::nfd::FacePersistency persistency,
                                       ::ndn::nfd::LinkType linkType)
  : m_node(node)
  , m_netDevice(netDevice)
  , m_remoteAddress(remoteAddress)
  , m_nodeType(caf::NODE_TYPE_NONE)
  , m_filter(filter)
{
  this->setLocalUri(FaceUri(constructFaceUri(netDevice)));
  this->setRemoteUri(FaceUri(constructFaceUri(remoteAddress)));
  this->setScope(scope);
  this->setPersistency(persistency);
  this->setLinkType(linkType);
  caf::NodeTypeHeader header;
  this->setMtu(m_netDevice->GetMtu() - header.GetSerializedSize()); // use netDevice's MTU - header size
  
  Ptr<caf::Context> ctx = node->GetObject<caf::Context>();
  NS_ABORT_MSG_IF(!ctx, "CafContext must be aggregated to the node before starting NDN.");
  m_nodeType = ctx->GetNodeType();

  // Get send queue capacity for congestion marking
  PointerValue txQueueAttribute;
  if (m_netDevice->GetAttributeFailSafe("TxQueue", txQueueAttribute)) {
    Ptr<ns3::QueueBase> txQueue = txQueueAttribute.Get<ns3::QueueBase>();
    // must be put into bytes mode queue

    auto size = txQueue->GetMaxSize();
    if (size.GetUnit() == BYTES) {
      this->setSendQueueCapacity(size.GetValue());
    }
    else {
      // don't know the exact size in bytes, guessing based on "standard" packet size
      this->setSendQueueCapacity(size.GetValue() * 1500);
    }
  }

  NS_LOG_FUNCTION(this << "Creating an ndnSIM transport instance for netDevice with URI"
                  << this->getLocalUri());

  NS_ASSERT_MSG(m_netDevice != 0, "NetDeviceFace needs to be assigned a valid NetDevice");

  m_node->RegisterProtocolHandler(MakeCallback(&WifiNetDeviceTransport::receiveFromNetDevice, this),
                                  L3Protocol::ETHERNET_FRAME_TYPE, m_netDevice,
                                  true /*promiscuous mode*/);
}

WifiNetDeviceTransport::~WifiNetDeviceTransport()
{
  NS_LOG_FUNCTION_NOARGS();
}

ssize_t
WifiNetDeviceTransport::getSendQueueLength()
{
  PointerValue txQueueAttribute;
  if (m_netDevice->GetAttributeFailSafe("TxQueue", txQueueAttribute)) {
    Ptr<ns3::QueueBase> txQueue = txQueueAttribute.Get<ns3::QueueBase>();
    return txQueue->GetNBytes();
  }
  else {
    return nfd::face::QUEUE_UNSUPPORTED;
  }
}

void
WifiNetDeviceTransport::doClose()
{
  NS_LOG_FUNCTION(this << "Closing transport for netDevice with URI"
                  << this->getLocalUri());

  // set the state of the transport to "CLOSED"
  this->setState(nfd::face::TransportState::CLOSED);
}

void
WifiNetDeviceTransport::doSend(const Block& packet)
{
  // convert NFD packet to NS3 packet
  BlockHeader header(packet);
  caf::NodeTypeHeader nodeType(m_nodeType);

  Ptr<ns3::Packet> ns3Packet = Create<ns3::Packet>();
  ns3Packet->AddHeader(header);
  ns3Packet->AddHeader(nodeType);

  NS_LOG_DEBUG("Sending packet(size="<< ns3Packet->GetSize() << ") "
               << "from netDevice with URI" << this->getLocalUri());

  // send the NS3 packet
  m_netDevice->Send(ns3Packet, m_remoteAddress,
                    L3Protocol::ETHERNET_FRAME_TYPE);
}

inline bool
dropPacket(caf::TransportFilter filter, caf::NodeType n1, caf::NodeType n2)
{
  /*
   * when filter == ALLOW_SAME (0)
   *    if n1 == n2 then x = 1 (return 0)
   *    if n1 != n2 then x = 0 (return 1)
   * when filter == ALLOW_DIFFERENT (1)
   *    if n1 == n2 then x = 1 (return 1)
   *    if n1 != n2 then x = 0 (return 0)
   */
  return (n1 == n2) == filter;
}

// callback
void
WifiNetDeviceTransport::receiveFromNetDevice(Ptr<NetDevice> device,
                                         Ptr<const ns3::Packet> p,
                                         uint16_t protocol,
                                         const Address& from, const Address& to,
                                         NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION(device << p << protocol << from << to << packetType);
  
  if (to != m_remoteAddress) {
    NS_LOG_LOGIC("Dropping packet: Destination address " << to << " does not match remote address " << m_remoteAddress);    
    return;
  }

  // Convert NS3 packet to NFD packet
  Ptr<ns3::Packet> packet = p->Copy();

  caf::NodeTypeHeader senderNodeType;
  packet->RemoveHeader(senderNodeType);
  
  if (m_filter != caf::ALLOW_ALL && dropPacket(m_filter, m_nodeType, senderNodeType.GetNodeType())) {
    NS_LOG_LOGIC("Dropping packet: Filter rejected");
    return;
  }

  BlockHeader header;
  packet->RemoveHeader(header);

  this->receive(std::move(header.getBlock()));
}

Ptr<NetDevice>
WifiNetDeviceTransport::GetNetDevice() const
{
  return m_netDevice;
}

} // namespace ndn
} // namespace ns3
