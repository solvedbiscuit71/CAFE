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
checkInside(const ns3::Node& node, const ns3::caf::ZoR& zor)
{
  using namespace ns3;

  Ptr<MobilityModel> mobility = node.GetObject<MobilityModel>();
  if (mobility != nullptr) {
    Vector v = mobility->GetPosition();
    return zor.contains({static_cast<float>(v.x), static_cast<float>(v.y)});
  }
  return false;
}

bool
Forwarder::OnIncomingAlert(const Data& data, const FaceEndpoint& ingress)
{
  NFD_LOG_DEBUG("OnIncomingAlert: in=" << ingress << " alert=" << data.getName());

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
    NFD_LOG_DEBUG("OnIncomingAlert: either node or ctx is nullptr; decision=drop");
    return false;
  }

  // if ZoR is nullptr, then alert is local scoped
  if (zor == nullptr) {
    NFD_LOG_DEBUG("OnIncomingAlert: zor is nullptr; decision=drop");
    return true;
  }
  
  if (zor->getType() == caf::NEIGHBOR) {
    // from a NON_LOCAL face then don't forward
    if (ingress.face.getScope() == ::ndn::nfd::FACE_SCOPE_NON_LOCAL) {
      NFD_LOG_DEBUG("NeighborHandler: incoming face is non-local; decision=drop");
      return true;
    }
    NFD_LOG_DEBUG("NeighborHandler: incoming face is local; decision=broadcast");
    
    // set sender info:
    // In case of hello message, sender position can be used for
    // predictive handoff where packet are delayed until the handover occurs
    data.setTag(make_shared<lp::SenderTypeTag>(ctx->GetNodeType()));
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    if (mobility != nullptr) {
      Vector v = mobility->GetPosition();
      data.setTag(make_shared<lp::SenderPositionTag>(
        std::make_tuple(v.x, v.y, v.z)
      ));
    }
    
    // forward to all NON_LOCAL faces
    for (auto& face: m_faceTable) {
      if (face.getScope() == ::ndn::nfd::FACE_SCOPE_NON_LOCAL) {
        this->onOutgoingData(data, face);
      }
    }
    return false;
  }
  
  // ZoR is geo-graphic (circle, polygon, composite, ...)
  // delegate forwarding to node-specific handler
  switch (ctx->GetNodeType()) {
    case caf::NODE_TYPE_VEHICLE:
      return AlertVehicleHandler(data, ingress, *zor, *node, *ctx);
    case caf::NODE_TYPE_RSU:
      return AlertRsuHandler(data, ingress, *zor, *node, *ctx);
    case caf::NODE_TYPE_BACKBONE:
    case caf::NODE_TYPE_NONE:
      break;
  }

  // cancel further processing the alert packet
  return false;
}

bool
Forwarder::AlertVehicleHandler(const Data& data, const FaceEndpoint& ingress, 
                               const ns3::caf::ZoR& zor, const ns3::Node& node, ns3::caf::Context& ctx)
{
  using namespace ns3;
  NFD_LOG_DEBUG("VehicleHandler: decision=drop");
  
  return checkInside(node, zor);
}

bool
Forwarder::AlertRsuHandler(const Data& data, const FaceEndpoint& ingress, 
                           const ns3::caf::ZoR& zor, const ns3::Node& node, ns3::caf::Context& ctx)
{
  using namespace ns3;

  // is duplicate?
  auto as = ctx.GetAlertStore();
  if (!as->InsertOrUpdate(data.getName())) {
    NFD_LOG_DEBUG("RsuHandler: duplicate alert; decision=drop");
    return false;
  }

  auto dstNodesTag = data.getTag<lp::DestinationNodesTag>();
  if (dstNodesTag == nullptr) {
    dstNodesTag = make_shared<lp::DestinationNodesTag>();
    dstNodesTag->set(caf::ComputeDestinationNodes(*ctx.GetPositionInfo(), ctx.GetTxRadius(), zor));
  }
  
  // if node in dstNodesTag then forward the message to V2I face
  // TODO: what if there are unreachable node? we should send via V2I again
  if (dstNodesTag->contains(node.GetId())) {
    auto v2i = ctx.GetFaceIdFor(ctx.V2I_FACE);
    if (v2i != 0) {
      auto& face = *m_faceTable.get(v2i);
      data.removeTag<lp::DestinationNodesTag>();

      NFD_LOG_DEBUG("RsuHandler: forward to V2I(id="<< v2i <<")");
      this->onOutgoingData(data, face);
      
      // remove current nodeId from destination list
      dstNodesTag->remove(node.GetId());
    }
  }
  
  // MIRA (MST Based Inter Routing Algorithm) is applied to compute all outgoing faces
  // TODO: we should have a fallback face (i.e. V2I) for unreachable node
  auto entries = caf::Mira(*ctx.GetRoutingInfo(), dstNodesTag->get(), node.GetId());
  
  for (auto& entry: entries) {
    dstNodesTag->set(entry.second);
    data.setTag(dstNodesTag);
    
    auto& face = *m_faceTable.get(entry.first);

    NFD_LOG_DEBUG("RsuHandler: forward to face(" << face.getId() << ")");
    this->onOutgoingData(data, face);
  }

  return checkInside(node, zor);
}

}