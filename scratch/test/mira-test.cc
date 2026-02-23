#include <iostream>
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <limits>
#include <functional>

#include "model/caf-routing.hpp"

#include "model/caf-zor.hpp"
#include "test.h"

using namespace ns3::caf;

// ------------------------------------------------------
// Demo
// ------------------------------------------------------ 

void print(const DestinationNodes& nodes) {
    for (auto i: nodes) {
        std::cout << i << ',';
    }
}

void
simulate(Graph& graph, DestinationNodes& nodes, NodeId sourceId)
{
    if (nodes.find(sourceId) != nodes.end()) {
        // forward to internal app face
        std::cout << "  destination nodeId=" << sourceId << " has reached" << std::endl;
        nodes.erase(sourceId);
    }

    // --- logging: start ---
    std::cout << "nodeId=" << sourceId << " destinationNodes="; print(nodes);
    std::cout << std::endl;
    // --- logging: end ---
    std::unordered_map<FaceId, DestinationNodes> mapping = Mira(graph, nodes, sourceId);
    
    for (auto& it: mapping) {
        // forward to outgoing face
        std::cout << "  forward to faceId=" << it.first << " destinationNodes="; print(it.second);
        std::cout << std::endl;
    }
    std::cout<<std::endl;
    
    for (auto& it: mapping) {
        for (auto& face: graph[sourceId]) {
            if (face.faceId == it.first) {
                simulate(graph, it.second, face.remoteNodeId);
                break;
            }
        }
    }
}

void
MiraTest()
{
    Graph G;
    
    G[1].push_back(Face{101, 2, 1});
    G[1].push_back(Face{102, 3, 6});

    G[2].push_back(Face{101, 1, 1});
    G[2].push_back(Face{102, 4, 2});

    G[3].push_back(Face{101, 1, 6});
    G[3].push_back(Face{102, 4, 2});

    G[4].push_back(Face{101, 2, 2});
    G[4].push_back(Face{102, 3, 2});

    G[6].push_back(Face{101, 7, 1});
    G[6].push_back(Face{102, 8, 2});

    G[7].push_back(Face{101, 6, 1});
    G[8].push_back(Face{101, 6, 2});
    
    NodePosition P;
    P[1] = {100,100};
    P[2] = {200,100};
    P[3] = {100,0};
    P[4] = {200,0};
    P[5] = {0,0};
    P[6] = {300,100};
    P[7] = {400,100};
    P[8] = {300,0};
    
    auto zor = std::make_shared<PolygonZoR>(std::vector<Point>{
        {95,105},
        {205,105},
        {205,95},
        {95,95},
    });

    NodeId target[] = {5, 3, 8, 6};
    for (int i=0; i<4; i++) {
        auto startNode = target[i]; 
        
        auto dstNode = ComputeDestinationNodes(G, P, 50.0, startNode, *zor);

        // logging start
        std::cout << "Test #" << i << " startNode=" << startNode << ' ';
        std::cout << "destination nodes=";
        print(dstNode);
        std::cout << std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        // logging end
        
        simulate(G, dstNode, startNode);
    }
}