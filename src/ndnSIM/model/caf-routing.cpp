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

#include <limits>
#include <queue>

#include "ns3/log.h"
#include "caf-routing.hpp"
#include "caf-zor.hpp"

NS_LOG_COMPONENT_DEFINE("caf.Routing");

namespace ns3 {
namespace caf {

using CostNodePair = std::pair<Cost,NodeId>;

/**
 * @param graph should be stored on the node (we can use GlobalHelper to populate this; later we can use routing algorithms)
 * @param destinationNodes should be attached along the packet (usually a LP Header)
 * @param sourceId current node id
 * @return list of outgoing face id and their respective destinationNodes (which should be attached as LP header)
 */
std::unordered_map<FaceId,DestinationNodes>
Mira(const Graph& G, const DestinationNodes& destinationNodes, NodeId sourceId)
{
    // initialize distance
    const Cost MAX = std::numeric_limits<Cost>::max();
    const NodeId UNDEFINED = -1;

    std::unordered_map<NodeId, Cost> distance;
    std::unordered_map<NodeId, NodeId> parent;
    for (const auto& it: G) {
        NodeId nodeId = it.first;

        distance[nodeId] = MAX;
        parent[nodeId] = UNDEFINED;
    }
    distance[sourceId] = 0;
    
    // initialize parent

    // min-heap on distance, node
    std::priority_queue<CostNodePair,
                        std::vector<CostNodePair>,
                        std::greater<CostNodePair>> queue;
    
    queue.push({0, sourceId});
    
    // dijkstra
    while (!queue.empty()) {
        const CostNodePair top = queue.top();
        queue.pop();
        
        Cost cost = top.first;
        NodeId nodeId = top.second;
        Cost nodeCost = distance.at(nodeId);
        
        if (cost > nodeCost) continue;
        
        if (G.find(nodeId) == G.end()) continue;
        
        for (const auto& face: G.at(nodeId)) {

            Cost& remoteCost = distance.at(face.remoteNodeId);
            
            if (remoteCost > nodeCost + face.linkCost) {
                remoteCost = nodeCost + face.linkCost;
                parent[face.remoteNodeId] = nodeId;
                queue.push({remoteCost, face.remoteNodeId});
            }
        }
    }
    
    // compute next hop
    std::unordered_map<NodeId, DestinationNodes> ir;

    for (int nodeId: destinationNodes) {
        if (distance.at(nodeId) == MAX) continue;
        
        NodeId cNode = nodeId;
        NodeId pNode;
        
        while ((pNode = parent.at(cNode)) != UNDEFINED && pNode != sourceId) {
            cNode = pNode;
        }
        
        if (pNode == sourceId) {
            ir[cNode].insert(nodeId);
        }
    }
    
    // unordered_map NodeId to FaceId
    std::unordered_map<FaceId,DestinationNodes> result;
    for (const auto& it: ir) {
        NodeId nodeId = it.first;
        const DestinationNodes& dNodes = it.second;

        if (G.find(nodeId) == G.end()) continue;
        for (const auto& face: G.at(sourceId)) {
            if (face.remoteNodeId == nodeId) {
                result[face.faceId] = dNodes;
                break;
            }
        }
    }

    return result;
}

/**
 * @param rsuPosition list of RSU (id, position)
 * @param txRadius transmission radius of RSU
 * @param zor Zone of Relevance object
 * @return list of rsu id which covers the given \p zor object
 */
DestinationNodes
ComputeDestinationNodes(const NodePosition& rsuPositions, float txRadius, const ZoR& zor)
{
    DestinationNodes dstNodes;
    
    for (const auto& it: rsuPositions) {
        if (zor.coveredBy(it.second, txRadius)) {
            dstNodes.emplace(it.first);
        }
    }

    return dstNodes;
}

/**
 * @param G network graph
 * @param rsuPosition list of RSU (id, position)
 * @param txRadius transmission radius of RSU
 * @param startNode current Node id
 * @param zor Zone of Relevance object
 * @return list of rsu id which covers the given \p zor or rsu's which are closest to \p zor
 */
DestinationNodes
ComputeDestinationNodes(const Graph& G, const NodePosition& rsuPositions, float txRadius, NodeId startNode, const ZoR& zor)
{
    DestinationNodes dstNodes;

    /**
     * Perform BFS traversal
     */
    std::unordered_set<NodeId> visited;
    std::queue<NodeId> que;
    
    NodeId nearestNode = std::numeric_limits<NodeId>::max();
    float minDist = INF;
    
    auto compute = [&](NodeId nodeId) {
        auto it = rsuPositions.find(nodeId);
        if (it != rsuPositions.end()) {
            if (zor.contains(it->second)) {
                dstNodes.emplace(nodeId);
            } else {
                auto dist = zor.distanceToBoundary(it->second);
                if (dist < minDist) {
                    minDist = dist;
                    nearestNode = nodeId;
                }
            }
        }
    };

    visited.insert(startNode);
    que.push(startNode);
    compute(startNode);

    while (!que.empty()) {
        uint32_t curr = que.front();
        que.pop();

        auto it = G.find(curr);
        if (it != G.end()) {
            for (const Face& face : it->second) {
                if (visited.find(face.remoteNodeId) == visited.end()) {
                    // unvisited node
                    visited.insert(face.remoteNodeId);
                    que.push(face.remoteNodeId);
                    compute(face.remoteNodeId);
                }
            }
        }
    }
    
    // If dstNodes is empty -> No RSU in the network is inside the ZoR
    // Fallback to nearest RSU
    if (dstNodes.empty()) {
        dstNodes.emplace(nearestNode);
    }

    return dstNodes;
}

} // namespace caf
} // namespace ns3
