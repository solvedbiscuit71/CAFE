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
#include "ns3/mac48-address.h"

namespace ns3 {
namespace caf {

TypeId 
Context::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::CafContext")
    .SetGroupName("Caf")
    .SetParent<Object>()
    .AddConstructor<Context>()
  ;
  return tid;
}

void
setupContext(Ptr<Node> node, std::function<void(Ptr<Context>)> configCallback)
{
    ns3::Ptr<Context> context = CreateObject<Context>();
    configCallback(context);
    node->AggregateObject(context);
}

void
setupContext(NodeContainer container, 
                std::function<void(Ptr<Context>)> configCallback) 
{
    for (auto it = container.Begin(); it != container.End(); ++it) {
        ns3::Ptr<Node> node = *it;
        ns3::Ptr<Context> context = CreateObject<Context>();
        configCallback(context);
        node->AggregateObject(context);
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

const ns3::Address MulticastGroup::MULTICAST_ALL = ns3::Mac48Address("01:00:5e:00:17:aa");
const ns3::Address MulticastGroup::MULTICAST_V2V = ns3::Mac48Address("01:00:5e:00:17:ab");
const ns3::Address MulticastGroup::MULTICAST_V2I = ns3::Mac48Address("01:00:5e:00:17:ac");   

} // namespace caf
} // namespace ns3
