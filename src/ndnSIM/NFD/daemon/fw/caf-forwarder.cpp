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

#include "face/face-endpoint.hpp"
#include "forwarder.hpp"

/**
 * ns3 namespace
 */
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/mobility-model.h"
#include "ns3/vector.h"
#include "ns3/ptr.h"
#include "model/caf-context.hpp"
#include "model/caf-zor.hpp"
#include "model/caf-routing.hpp"

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
    NFD_LOG_DEBUG("OnIncomingAlert in=" << ingress << " alert=" << data.getName()
                  << " decision=drop");
    return false;
  }

  // if ZoR is nullptr, then alert is local scoped
  if (zor == nullptr) {
    return true;
  }
  
  // delegate forwarding to node-specific handler
  switch (ctx->GetNodeType()) {
    case caf::NODE_TYPE_VEHICLE:
      AlertVehicleHandler(data, ingress, *zor, *node, *ctx);
      break;
    case caf::NODE_TYPE_RSU:
      AlertRsuHandler(data, ingress, *zor, *node, *ctx);
      break;
    case caf::NODE_TYPE_BACKBONE:
    case caf::NODE_TYPE_NONE:
      break;
  }

  // check whether inside ZoR
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
  if (mobility != nullptr) {
    Vector v = mobility->GetPosition();
    return zor->contains({static_cast<float>(v.x), static_cast<float>(v.y)});
  }
  return false;
}

void
Forwarder::AlertVehicleHandler(const Data& data, const FaceEndpoint& ingress, 
                               const ns3::caf::ZoR& zor, const ns3::Node& node, const ns3::caf::Context& ctx)
{
  using namespace ns3;
  NFD_LOG_DEBUG("VehicleHandler " << " alert=" << data.getName()
                << " decision=drop");
  
  return;
}

void
Forwarder::AlertRsuHandler(const Data& data, const FaceEndpoint& ingress, 
                           const ns3::caf::ZoR& zor, const ns3::Node& node, const ns3::caf::Context& ctx)
{
  using namespace ns3;
  NFD_LOG_DEBUG("RsuHandler " << " alert=" << data.getName()
                << " decision=forward to V2I");

  auto& v2i = *m_faceTable.get(ctx.GetFaceIdFor(caf::Context::V2I_FACE));
  this->onOutgoingData(data, v2i);
}

}