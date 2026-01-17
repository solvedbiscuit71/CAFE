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

#include "ndn-context.hpp"
#include "ns3/mac48-address.h"

namespace ns3 {

TypeId 
CafContext::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::CafContext")
    .SetParent<Object>()
    .SetGroupName("caf")
    .AddConstructor<CafContext>()
  ;
  return tid;
}

void
setupCafContext(Ptr<Node> node, std::function<void(Ptr<CafContext>)> configCallback)
{
    ns3::Ptr<CafContext> context = CreateObject<CafContext>();
    configCallback(context);
    node->AggregateObject(context);
}

void
setupCafContext(NodeContainer container, 
                std::function<void(Ptr<CafContext>)> configCallback) 
{
    for (auto it = container.Begin(); it != container.End(); ++it) {
        ns3::Ptr<Node> node = *it;
        ns3::Ptr<CafContext> context = CreateObject<CafContext>();
        configCallback(context);
        node->AggregateObject(context);
    }
}

const ns3::Address MulticastGroup::MULTICAST_ALL = ns3::Mac48Address("01:00:5e:00:17:aa");
const ns3::Address MulticastGroup::MULTICAST_V2V = ns3::Mac48Address("01:00:5e:00:17:ab");
const ns3::Address MulticastGroup::MULTICAST_V2I = ns3::Mac48Address("01:00:5e:00:17:ac");   

} // namespace ns3
