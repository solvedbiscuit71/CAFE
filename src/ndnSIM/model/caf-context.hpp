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

#ifndef CAF_CONTEXT_HPP
#define CAF_CONTEXT_HPP

#include "face/face-common.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/address.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object.h"

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include "caf-routing.hpp"

namespace ns3 {
namespace caf {

enum NodeType : uint8_t {
  NODE_TYPE_NONE      = 0,
  NODE_TYPE_VEHICLE   = 1,
  NODE_TYPE_RSU       = 2,
  NODE_TYPE_BACKBONE  = 3,
};

enum NodeStatus : uint8_t {
  NODE_STATUS_UNKNOWN  = 0,
  NODE_STATUS_ACTIVE   = 1,
  NODE_STATUS_INACTIVE = 2,
};

enum TransportFilter : uint8_t {
  ALLOW_ALL       = 0,
  ALLOW_SAME      = 1,
  ALLOW_DIFFERENT = 2,
};

static const double defaultTxRadius = 50.0;

using FaceIdContextMap = boost::bimap<
    boost::bimaps::unordered_set_of<nfd::face::FaceId>, 
    boost::bimaps::unordered_set_of<std::string>
>;

class Context : public Object {
public:
  static TypeId GetTypeId (void);

  static const std::string V2V_FACE;
  static const std::string V2I_FACE;
  static const std::string V2X_FACE;
  static const std::string UNDEFINED_FACE;
  
  Context() 
    : m_type(NODE_TYPE_NONE), m_status(NODE_STATUS_UNKNOWN), m_receivedHello(false), m_txRadius(defaultTxRadius) {}
  
  void SetNodeType(NodeType type) { m_type = type; }
  NodeType GetNodeType() const { return m_type; }
  
  void SetNodeStatus(NodeStatus status) { m_status = status; }
  NodeStatus GetNodeStatus() const { return m_status; }
  
  void SetReceivedHello(bool receivedHello) { m_receivedHello = receivedHello; }
  bool GetReceivedHello() const { return m_receivedHello; }
  
  void SetFaceIdContext(nfd::face::FaceId faceId, std::string context);
  nfd::face::FaceId GetFaceIdFor(std::string context) const;
  std::string GetContextFor(nfd::face::FaceId faceId) const;
  
  void SetTxRadius(double txRadius) { m_txRadius = txRadius; }
  double GetTxRadius() const { return m_txRadius; }
  
  static Graph* GetRoutingInfo();
  static NodePosition* GetPositionInfo();

private:
  NodeType m_type;
  NodeStatus m_status;
  FaceIdContextMap m_faceContextMap;
  bool m_receivedHello;
  double m_txRadius;
};

void
setupContext(Node node, std::function<void(Ptr<Context>)> configCallback);

void
setupContext(NodeContainer container, std::function<void(Ptr<Context>)> configCallback);

class NodeTypeHeader : public Header {
public:
  NodeTypeHeader()
    : m_role(NODE_TYPE_NONE) { }
  NodeTypeHeader(NodeType role)
    : m_role(role) { }
  virtual ~NodeTypeHeader() {}

  void SetNodeType(NodeType role) { m_role = role; }
  NodeType GetNodeType() const { return m_role; }

  static TypeId GetTypeId(void);

  virtual TypeId GetInstanceTypeId(void) const override { return GetTypeId(); }
  virtual void Print(std::ostream &os) const override { os << "NodeType=" << m_role; }
  virtual uint32_t GetSerializedSize(void) const override { return 1; }
  virtual void Serialize(Buffer::Iterator start) const override;
  virtual uint32_t Deserialize(Buffer::Iterator start) override;

private:
  NodeType m_role;
};

class MulticastGroup {
public:
    static const ns3::Address MULTICAST_V2V;
    static const ns3::Address MULTICAST_V2I;
    static const ns3::Address MULTICAST_V2X;
};

} // namespace caf
} // namespace ns3

#endif // CAF_CONTEXT_HPP
