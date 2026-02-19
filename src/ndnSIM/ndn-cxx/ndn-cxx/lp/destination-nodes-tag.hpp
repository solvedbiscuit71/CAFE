/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#ifndef NDN_CXX_LP_DESTINATION_NODES_TAG_HPP
#define NDN_CXX_LP_DESTINATION_NODES_TAG_HPP

#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ndn-cxx/tag.hpp"
#include <cstdint>
#include <unordered_set>

namespace ndn {
namespace lp {

/**
 * \brief represents a DestinationNodesTag header field
 */
class DestinationNodesTag : public Tag
{
public:
  static constexpr int
  getTypeId() noexcept
  {
    return 0x70000002;
  }

  DestinationNodesTag() = default;

  explicit
  DestinationNodesTag(std::unordered_set<uint32_t> nodes)
    : m_nodes(nodes)
  {
  }

  explicit
  DestinationNodesTag(const Block& block);

  /**
   * \brief prepend DestinationNodesTag to encoder
   */
  template<encoding::Tag TAG>
  size_t
  wireEncode(EncodingImpl<TAG>& encoder) const;

  /**
   * \brief encode DestinationNodesTag into wire format
   */
  const Block&
  wireEncode() const;

  /**
   * \brief get DestinationNodesTag from wire format
   */
  void
  wireDecode(const Block& wire);

public:
  /**
   * \return return nodes
   */
  const std::unordered_set<uint32_t>&
  get() const
  {
    return m_nodes;
  }

  /**
   * \brief set nodes
   */
  void
  set(std::unordered_set<uint32_t> nodes) { m_nodes = nodes; }

  /**
   * \brief mutate nodes
   */
  void
  add(uint32_t nodeId) { m_nodes.emplace(nodeId); }
  
  void
  remove(uint32_t nodeId) { m_nodes.erase(nodeId); }
  
  void
  clear() { m_nodes.clear(); }
  
private:
  std::unordered_set<uint32_t> m_nodes;
  mutable Block m_wire;
};

} // namespace lp
} // namespace ndn

#endif // NDN_CXX_LP_GEOTAG_HPP
