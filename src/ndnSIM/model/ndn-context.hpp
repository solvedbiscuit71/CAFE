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

#ifndef NDNSIM_NDN_CONTEXT_H
#define NDNSIM_NDN_CONTEXT_H

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/address.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object.h"

namespace ns3 {
  
class CafContext : public Object {
public:
  enum NodeType : uint8_t {
    NODE_TYPE_NONE      = std::numeric_limits<uint8_t>::max(),
    NODE_TYPE_VEHICLE   = 0,
    NODE_TYPE_RSU       = 1,
    NODE_TYPE_BACKBONE  = 2,
  };

  static TypeId GetTypeId (void);
  
  CafContext() : m_type(NODE_TYPE_NONE) {}
  
  void SetNodeType(NodeType type) { m_type = type; }
  NodeType GetNodeType() const { return m_type; }

private:
  NodeType m_type;
};

void
setupCafContext(Node node, std::function<void(Ptr<CafContext>)> configCallback);

void
setupCafContext(NodeContainer container, std::function<void(Ptr<CafContext>)> configCallback);

class NodeTypeHeader : public Header {
public:
  NodeTypeHeader()
    : m_role(CafContext::NODE_TYPE_NONE) { }
  NodeTypeHeader(CafContext::NodeType role)
    : m_role(role) { }
  virtual ~NodeTypeHeader() {}

  void SetNodeType(CafContext::NodeType role) { m_role = role; }
  CafContext::NodeType GetNodeType() const { return m_role; }

  static TypeId GetTypeId(void);

  virtual TypeId GetInstanceTypeId(void) const override { return GetTypeId(); }
  virtual void Print(std::ostream &os) const override { os << "NodeType=" << m_role; }
  virtual uint32_t GetSerializedSize(void) const override { return 1; }
  virtual void Serialize(Buffer::Iterator start) const override;
  virtual uint32_t Deserialize(Buffer::Iterator start) override;

private:
  CafContext::NodeType m_role;
};

class MulticastGroup {
public:
    static const ns3::Address MULTICAST_ALL;
    static const ns3::Address MULTICAST_V2V;
    static const ns3::Address MULTICAST_V2I;
};
}

#endif // NDNSIM_NDN_CONTEXT_H
