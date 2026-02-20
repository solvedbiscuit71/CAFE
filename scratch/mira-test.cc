#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <limits>
#include <functional>

#include "model/caf-routing.hpp"

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
    // --- logging: start ---
    std::unordered_map<FaceId, DestinationNodes> mapping = Mira(graph, nodes, sourceId);
    std::cout << "nodeId=" << sourceId << " destinationNodes="; print(nodes);
    std::cout << std::endl;
    // --- logging: end ---
    
    bool isDestination = false;
    for (auto nId: nodes) {
        isDestination = isDestination || nId == sourceId;
    }
    
    if (isDestination) {
        // forward to internal app face
        std::cout << "  destination nodeId=" << sourceId << " has reached" << std::endl;
    }
    
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

int
main()
{
    Graph G;
    
    G[1].push_back(Face{101,2,1});
    G[1].push_back(Face{102, 3, 3});
    G[1].push_back(Face{103, 4, 2});

    G[2].push_back(Face{101, 7, 3});
    G[2].push_back(Face{102, 1, 1});
    G[2].push_back(Face{103, 6, 1});
    
    G[3].push_back(Face{101, 1, 3});
    G[3].push_back(Face{102, 6, 4});
    G[3].push_back(Face{103, 5, 2});

    G[4].push_back(Face{101, 1, 2});
   G[4].push_back(Face{102, 5, 1});

    G[5].push_back(Face{101, 3, 2});
    G[5].push_back(Face{102, 4, 1});

    G[6].push_back(Face{101, 2, 1});
    G[6].push_back(Face{102, 3, 4});

    G[7].push_back(Face{101, 2, 3});
    
    
    DestinationNodes dNodes{2, 7, 3, 5};
    simulate(G, dNodes, 1);
    return 0;
}