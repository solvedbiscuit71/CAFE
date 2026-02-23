/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/common.hpp"
#include "face/face-endpoint.hpp"
#include "forwarder.hpp"

/**
 * ns3 namespace
 */
#include "lp/destination-nodes-tag.hpp"
#include "lp/sender-position-tag.hpp"
#include "lp/tags.hpp"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/mobility-model.h"
#include "ns3/vector.h"
#include "ns3/ptr.h"
#include "model/caf-context.hpp"
#include "model/caf-zor.hpp"
#include "model/caf-routing.hpp"
#include <memory>
#include <tuple>

namespace nfd {

NS_LOG_COMPONENT_DEFINE("caf.Forwarder");

// @assume isAlert(data) == true
static std::unique_ptr<ns3::caf::ZoR>
extractZoR(const Data& data)
{
  int i;
  const auto& name = data.getName();
  for (i=name.size()-1; i>=0; --i) {
    if (name.get(i).isZoR()) {
      break;
    }
  }
  // has ZoR name component
  if (i != -1) {
    return name.get(i).toZoR();
  }
  return nullptr;
}

static bool
checkInside(ns3::Ptr<ns3::Node> node, const ns3::caf::ZoR& zor)
{
  using namespace ns3;

  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
  if (mobility != nullptr) {
    Vector v = mobility->GetPosition();
    return zor.contains({static_cast<float>(v.x), static_cast<float>(v.y)});
  }
  return false;
}

static ns3::caf::Point
convertToPoint(ns3::Vector v) {
  return ns3::caf::Point{static_cast<float>(v.x), static_cast<float>(v.y)};
}

static ns3::caf::Point
convertToPoint(std::tuple<double, double, double> v) {
  auto [x,y,z] = v;
  return ns3::caf::Point{static_cast<float>(x), static_cast<float>(y)};
}

bool
Forwarder::OnIncomingAlert(const Data& data, const FaceEndpoint& ingress)
{
  using namespace ns3;

  // extract ZoR
  auto zor = extractZoR(data);

  // resolve the ns-3 Node and Context
  uint32_t nodeId = Simulator::GetContext();

  Ptr<Node> node = nullptr;
  Ptr<caf::Context> ctx = nullptr;

  if (nodeId != 0xffffffff) { 
    node = NodeList::GetNode(nodeId);
    if (node != nullptr) {
      ctx = node->GetObject<caf::Context>();
    }
  }

  // guard condition
  if (!node || !ctx) {
    NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=drop reason='Either node or ctx is nullptr'");
    return false;
  }

  // if ZoR is nullptr, then alert is local scoped
  if (zor == nullptr) {
    NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=drop reason='Zor is nullptr'");
    return true;
  }
  
  if (zor->getType() == caf::NEIGHBOR) {
    // from a NON_LOCAL face then don't forward
    if (ingress.face.getScope() == ::ndn::nfd::FACE_SCOPE_NON_LOCAL) {
      NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=drop reason='Neighbor zor'");
      return true;
    }
    
    NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=broadcast");
    // forward to all NON_LOCAL faces
    for (auto& face: m_faceTable) {
      if (face.getScope() == ::ndn::nfd::FACE_SCOPE_NON_LOCAL) {
        this->OnOutgoingAlert(data, face, node, ctx);
      }
    }
    return false;
  }
  
  // ZoR is geo-graphic (circle, polygon, composite, ...)
  // delegate forwarding to node-specific handler
  switch (ctx->GetNodeType()) {
    case caf::NODE_TYPE_VEHICLE:
      return AlertVehicleHandler(data, ingress, *zor, node, ctx);
    case caf::NODE_TYPE_RSU:
      return AlertRsuHandler(data, ingress, *zor, node, ctx);
    case caf::NODE_TYPE_BACKBONE:
    case caf::NODE_TYPE_NONE:
      break;
  }

  // cancel further processing the alert packet
  return false;
}

void
Forwarder::OnOutgoingAlert(const Data& data, Face& egress, ns3::Ptr<ns3::Node> node, ns3::Ptr<ns3::caf::Context> ctx)
{
  using namespace ns3;
  NFD_LOG_DEBUG("out=" << egress.getId() << " alert=" << data.getName());

  data.setTag(make_shared<lp::SenderTypeTag>(ctx->GetNodeType()));
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
  if (mobility != nullptr) {
    Vector v = mobility->GetPosition();
    data.setTag(make_shared<lp::SenderPositionTag>(
      std::make_tuple(v.x, v.y, v.z)
    ));
  }
  
  this->onOutgoingData(data, egress);
}

void
Forwarder::DeferredOutgoingAlert(shared_ptr<const Data> data, shared_ptr<Face> egress, ns3::Ptr<ns3::Node> node, ns3::Ptr<ns3::caf::Context> ctx)
{
  NFD_LOG_DEBUG("timer expired");

  // mark schedule tranmission as completed
  auto registry = ctx->GetDeferredRegistry();
  registry->Complete(data->getName());

  this->OnOutgoingAlert(*data, *egress, node, ctx);
}

bool
Forwarder::AlertVehicleHandler(const Data& data, const FaceEndpoint& ingress, 
                               const ns3::caf::ZoR& zor, ns3::Ptr<ns3::Node> node, ns3::Ptr<ns3::caf::Context> ctx)
{
  using namespace ns3;
  
  // is duplicate?
  const Name& name = data.getName();
  auto as = ctx->GetAlertStore();
  if (!as->InsertOrUpdate(name)) {
    // cancel schedule transmission
    auto registry = ctx->GetDeferredRegistry();
    if (registry->IsRegistered(name)) {
      NFD_LOG_DEBUG("timer cancelled");
      registry->Cancel(name);
    }

    NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=drop reason='Duplication alert'");
    return false;
  }
  
  // check whether RSU available?
  if (ctx->GetReceivedHello()) {
    auto v2i = ctx->GetFaceIdFor(ctx->V2I_FACE);
    if (v2i != 0) {
      auto& face = *m_faceTable.get(v2i);
      data.removeTag<lp::DestinationNodesTag>();

      NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=forward");
      this->OnOutgoingAlert(data, face, node, ctx);
    }
    return checkInside(node, zor);
  }
  
  // extract sender and node position
  caf::Point senderPos, nodePos;

  auto mobility = node->GetObject<MobilityModel>();
  if (mobility != nullptr) {
    nodePos = convertToPoint(mobility->GetPosition());
  }

  auto senderPosTag = data.getTag<lp::SenderPositionTag>();
  // if senderPosTag is not present, forward to V2V immediately
  if (senderPosTag == nullptr) {
    auto v2v = ctx->GetFaceIdFor(ctx->V2V_FACE);
    if (v2v != 0) {
      auto& face = *m_faceTable.get(v2v);
      NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=forward");
      this->OnOutgoingAlert(data, face, node, ctx);
    }
    return zor.contains(nodePos);
  } else {
    senderPos = convertToPoint(senderPosTag->getPos());
  }
  
  if (!zor.contains(nodePos)) {
    if (zor.contains(senderPos) || zor.distanceToBoundary(senderPos) < zor.distanceToBoundary(nodePos)) {
      NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=drop reason='Sender is closer'");
      return false;
    }
  }

  // compute tMax
  auto tTx = (data.wireEncode().size() * 8) / ctx->GetTxRate(); // transmission delay
  auto tProp = ctx->GetTxRadius() / (3 * 1e8);                  // propagation delay
  auto tBuffer = 0.002;                                         // lower-layer headers + CSMA/CA backoff + queueing (empirical)
  auto tMax_in_ms = (tTx + tProp + tBuffer) * 1e3;

  // compute deferred delay
  auto dist = nodePos.distanceFromPoint(senderPos);
  auto delay_in_ms = 2 * tMax_in_ms * std::clamp((1 - dist / ctx->GetTxRadius()), 0.0, 1.0); // 2 * ( .. ) because RTT
  
  NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=deferred until "<<delay_in_ms<<"ms");

  auto v2v = ctx->GetFaceIdFor(ctx->V2V_FACE);
  if (v2v != 0) {
    auto& face = *m_faceTable.get(v2v);

    EventId event = Simulator::Schedule(ns3::NanoSeconds(delay_in_ms * 1e6), 
      &Forwarder::DeferredOutgoingAlert, this, data.shared_from_this(), face.shared_from_this(), node, ctx);
    
    auto registry = ctx->GetDeferredRegistry();
    registry->Register(name, event);
  }
  return zor.contains(nodePos);
}

bool
Forwarder::AlertRsuHandler(const Data& data, const FaceEndpoint& ingress, 
                           const ns3::caf::ZoR& zor, ns3::Ptr<ns3::Node> node, ns3::Ptr<ns3::caf::Context> ctx)
{
  using namespace ns3;

  // is duplicate?
  auto as = ctx->GetAlertStore();
  if (!as->InsertOrUpdate(data.getName())) {
    NFD_LOG_DEBUG("in=" << ingress << " alert=" << data.getName() << " decision=drop reason='Duplicate alert'");
    return false;
  }

  auto dstNodesTag = data.getTag<lp::DestinationNodesTag>();
  if (dstNodesTag == nullptr) {
    dstNodesTag = make_shared<lp::DestinationNodesTag>();
    
    // if network has RSU inside ZoR, return them
    // otherwise return the nearest RSU in the network (it can be the current node)
    dstNodesTag->set(caf::ComputeDestinationNodes(*ctx->GetRoutingInfo(),*ctx->GetPositionInfo(), ctx->GetTxRadius(), node->GetId(), zor));
  }
  
  // if node in dstNodesTag then forward the message to V2I face
  if (dstNodesTag->contains(node->GetId())) {
    auto v2i = ctx->GetFaceIdFor(ctx->V2I_FACE);
    if (v2i != 0) {
      auto& face = *m_faceTable.get(v2i);
      data.removeTag<lp::DestinationNodesTag>();

      this->OnOutgoingAlert(data, face, node, ctx);
      
      // remove current nodeId from destination list
      dstNodesTag->remove(node->GetId());
    }
  }
  
  // compute all outgoing faces for the dstNodes (using MIRA)
  if (!dstNodesTag->get().empty()) {
    auto entries = caf::Mira(*ctx->GetRoutingInfo(), dstNodesTag->get(), node->GetId());
    
    for (auto& entry: entries) {
      dstNodesTag->set(entry.second);
      data.setTag(dstNodesTag);
      
      auto& face = *m_faceTable.get(entry.first);

      this->OnOutgoingAlert(data, face, node, ctx);
    }
  }

  return checkInside(node, zor);
}

}