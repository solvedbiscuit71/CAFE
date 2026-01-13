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

namespace ns3 {

enum NodeType : uint8_t {
  NODE_TYPE_NONE      = std::numeric_limits<uint8_t>::max(),
  NODE_TYPE_VEHICLE   = 0,
  NODE_TYPE_RSU       = 1,
  NODE_TYPE_BACKBONE  = 2,
};

class MulticastGroup {
public:
    static const ns3::Address MULTICAST_ALL;
    static const ns3::Address MULTICAST_V2V;
    static const ns3::Address MULTICAST_V2I;
};
}

#endif // NDNSIM_NDN_CONTEXT_H
