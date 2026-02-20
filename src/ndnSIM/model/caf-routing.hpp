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

#ifndef CAF_ROUTING_HPP
#define CAF_ROUTING_HPP

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace ns3 {
namespace caf {
  
using FaceId = uint64_t;
using NodeId = uint32_t;
using Cost = uint32_t;

struct Face {
  FaceId faceId;
  NodeId remoteNodeId;
  Cost linkCost;
};

using Graph = std::unordered_map<NodeId, std::vector<Face>>;
using DestinationNodes = std::unordered_set<NodeId>;
using CostNodePair = std::pair<Cost,NodeId>;

std::unordered_map<FaceId,DestinationNodes>
Mira(Graph G, DestinationNodes destinationNodes, NodeId sourceId);

} // namespace caf
} // namespace ns3

#endif // CAF_ROUTING_HPP
