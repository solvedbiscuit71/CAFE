/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
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

#include "caf-context.hpp"

#include "face/face-common.hpp"
#include "ns3/log.h"
#include "ns3/mac48-address.h"

NS_LOG_COMPONENT_DEFINE("caf.Context");

namespace ns3 {
namespace caf {

bool
AlertStore::IsDuplicate(const ndn::Name& name) {
  return m_lookup.find(name) != m_lookup.end();
}

bool
AlertStore::InsertOrUpdate(const ndn::Name& name) {
  auto it = m_lookup.find(name);

  if (it != m_lookup.end()) {
    // Move the existing element from its current position to the front.
    m_order.splice(m_order.begin(), m_order, it->second);
    
    return false;
  }

  m_order.push_front(name);
  m_lookup[name] = m_order.begin();

  if (m_lookup.size() > m_maxSize) {
    evictOldest();
  }

  return true;
}

void 
AlertStore::evictOldest() {
  if (m_order.empty()) return;

  const ndn::Name& oldest = m_order.back();
  
  m_lookup.erase(oldest);
  m_order.pop_back();
} 

TypeId 
Context::GetTypeId(void)
{
  static TypeId tid = TypeId("caf::Context")
    .SetGroupName("Caf")
    .SetParent<Object>()
    .AddConstructor<Context>()
  ;
  return tid;
}

const std::string Context::V2V_FACE = "V2V";
const std::string Context::V2I_FACE = "V2I";
const std::string Context::V2X_FACE = "V2X";
const std::string Context::UNDEFINED_FACE = "UNDEFINED";

void
Context::SetFaceIdContext(nfd::face::FaceId faceId, std::string context)
{
  NS_LOG_DEBUG("Mapped faceId=" << faceId << " with context=" << context);
  m_faceContextMap.insert(FaceIdContextMap::value_type(faceId, context));
}

nfd::face::FaceId
Context::GetFaceIdFor(std::string context) const
{
  auto it = m_faceContextMap.right.find(context);
  if (it != m_faceContextMap.right.end()) {
    return it->second;
  }
  return 0;
}

std::string
Context::GetContextFor(nfd::face::FaceId faceId) const
{
  auto it = m_faceContextMap.left.find(faceId);
  if (it != m_faceContextMap.left.end()) {
    return it->second;
  }
  return UNDEFINED_FACE;
}

Graph*
Context::GetRoutingInfo()
{
  static Graph m_graph;
  return &m_graph;
}

NodePosition*
Context::GetPositionInfo()
{
  static NodePosition m_nodePositions;
  return &m_nodePositions;
}

void
setupContext(Ptr<Node> node, std::function<void(Ptr<Context>)> configCallback)
{
    Ptr<Context> ctx = node->GetObject<Context>();
    if (!ctx) {
      ctx = CreateObject<Context>();
      node->AggregateObject(ctx);
    }
    configCallback(ctx);
    NS_LOG_DEBUG("Configured context object on node ("<< node->GetId() <<")");
}

void
setupContext(NodeContainer container, 
                std::function<void(Ptr<Context>)> configCallback) 
{
  for (auto it = container.Begin(); it != container.End(); ++it) {
    setupContext(*it, configCallback);
  }
}

TypeId 
NodeTypeHeader::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::NodeTypeHeader")
    .SetParent<Header>()
    .SetGroupName("Caf")
    .AddConstructor<NodeTypeHeader>()
  ;
  return tid;
}

void 
NodeTypeHeader::Serialize(Buffer::Iterator start) const
{ 
  start.WriteU8(static_cast<uint8_t>(m_role)); 
}

uint32_t 
NodeTypeHeader::Deserialize(Buffer::Iterator start)
{
  m_role = static_cast<NodeType>(start.ReadU8());
  return 1;
}

const ns3::Address MulticastGroup::MULTICAST_V2V = ns3::Mac48Address("01:00:5e:00:17:aa");
const ns3::Address MulticastGroup::MULTICAST_V2I = ns3::Mac48Address("01:00:5e:00:17:ac");   
const ns3::Address MulticastGroup::MULTICAST_V2X = ns3::Mac48Address("01:00:5e:00:17:ae");

} // namespace caf
} // namespace ns3
