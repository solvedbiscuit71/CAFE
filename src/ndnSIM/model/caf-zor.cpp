/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors
 *and contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the
 *terms of the GNU General Public License as published by the Free Software
 *Foundation, either version 3 of the License, or (at your option) any later
 *version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY
 *WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 *A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see
 *<http://www.gnu.org/licenses/>.
 **/

#include "caf-zor.hpp"
#include "ns3/fatal-error.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("caf.ZoR");

namespace ns3 {
namespace caf {

float Point::distanceFromPoint(const Point &other) const {
  return std::hypot(x - other.x, y - other.y);
}

float Point::distanceFromLine(const Point &a, const Point &b) const {
  float dx = b.x - a.x;
  float dy = b.y - a.y;

  if (dx == 0 && dy == 0) {
    return distanceFromPoint(a);
  }

  float v = ((x - a.x) * dx + (y - a.y) * dy) / (dx * dx + dy * dy);
  float t = std::clamp(v, 0.0f, 1.0f);

  Point closest{a.x + t * dx, a.y + t * dy};
  return distanceFromPoint(closest);
}

size_t NeighborZoR::size() const {
  size_t size = 0;
  size += sizeof(ZoRType); // type
  return size;
}

void NeighborZoR::serialize(std::vector<uint8_t> &buf) const {
  uint8_t tag = getType();
  write<uint8_t>(buf, tag);
}

std::unique_ptr<NeighborZoR>
NeighborZoR::deserialize(const std::vector<uint8_t> &buf, size_t &offset) {
  return std::make_unique<NeighborZoR>();
}

size_t CircleZoR::size() const {
  size_t size = 0;
  size += center.size() + sizeof(float); // value
  size += sizeof(ZoRType);               // type
  return size;
}

bool CircleZoR::contains(const Point &p) const {
  return p.distanceFromPoint(this->center) <= radius;
}

bool CircleZoR::coveredBy(const Point &center, float radius) const {
  float d = center.distanceFromPoint(this->center);
  return d <= (this->radius + radius);
}

float CircleZoR::distanceToBoundary(const Point &p) const {
  float d = p.distanceFromPoint(this->center);
  return d > radius ? d - radius : 0.0;
}

void CircleZoR::serialize(std::vector<uint8_t> &buf) const {
  uint8_t tag = getType();
  write<uint8_t>(buf, tag);
  write<float>(buf, center.x);
  write<float>(buf, center.y);
  write<float>(buf, radius);
}

std::unique_ptr<CircleZoR>
CircleZoR::deserialize(const std::vector<uint8_t> &buf, size_t &offset) {
  float x = read<float>(buf, offset);
  float y = read<float>(buf, offset);
  float r = read<float>(buf, offset);
  return std::make_unique<CircleZoR>(Point{x, y}, r);
}

PolygonZoR::PolygonZoR(const std::vector<Point> &v) : vertices(v) {
  if (vertices.size() > std::numeric_limits<uint8_t>::max()) {
    NS_FATAL_ERROR("PolygonZoR: vertices.size() exceeds size limit.");
  }
}

PolygonZoR::PolygonZoR(std::vector<Point> &&v) : vertices(v) {
  if (vertices.size() > std::numeric_limits<uint8_t>::max()) {
    NS_FATAL_ERROR("PolygonZoR: vertices.size() exceeds size limit.");
  }
}

size_t PolygonZoR::size() const {
  size_t size = 0;
  size += vertices.size() * Point::size(); // value
  size += sizeof(uint8_t);                 // length
  size += sizeof(ZoRType);                 // type
  return size;
}

bool PolygonZoR::contains(const Point &p) const {
  bool inside = false;
  for (size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++) {
    const auto &vi = vertices[i];
    const auto &vj = vertices[j];
    if (((vi.y > p.y) != (vj.y > p.y)) &&
        (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y) + vi.x))
      inside = !inside;
  }
  return inside;
}

bool PolygonZoR::coveredBy(const Point &center, float radius) const {

  if (contains(center))
    return true;

  float dist = distanceToBoundary(center);
  return dist <= radius;
}

float PolygonZoR::distanceToBoundary(const Point &p) const {
  float minDist = INF;
  for (size_t i = 0; i < vertices.size(); ++i) {
    const Point &a = vertices[i];
    const Point &b = vertices[(i + 1) % vertices.size()];
    minDist = std::min(minDist, p.distanceFromLine(a, b));
  }
  return minDist;
}

void PolygonZoR::serialize(std::vector<uint8_t> &buf) const {
  uint8_t tag = getType();
  write(buf, tag);

  uint8_t n = vertices.size();
  write(buf, n);

  for (const auto &v : vertices) {
    write(buf, v.x);
    write(buf, v.y);
  }
}

std::unique_ptr<PolygonZoR>
PolygonZoR::deserialize(const std::vector<uint8_t> &buf, size_t &offset) {
  uint8_t n = read<uint8_t>(buf, offset);
  std::vector<Point> verts(n);
  for (uint8_t i = 0; i < n; ++i) {
    verts[i].x = read<float>(buf, offset);
    verts[i].y = read<float>(buf, offset);
  }
  return std::make_unique<PolygonZoR>(std::move(verts));
}

CompositeZoR::CompositeZoR(std::vector<std::unique_ptr<ZoR>> &&components)
    : components(std::move(components)) {
  if (components.size() > std::numeric_limits<uint8_t>::max()) {
    std::runtime_error("CompositeZoR: components.size() exceeds size limit.");
  }
}

size_t CompositeZoR::size() const {
  size_t size = 0;
  for (const auto& z : components) {
    size += z->size(); // value
  }
  size += sizeof(uint8_t); // length
  size += sizeof(ZoRType); // type
  return size;
}

bool CompositeZoR::append(std::unique_ptr<ZoR> zor) {
  if (components.size() + 1 > std::numeric_limits<uint8_t>::max()) {
    return false;
  }
  components.emplace_back(std::move(zor));
  return true;
}

bool CompositeZoR::contains(const Point &p) const {
  for (const auto &z : components)
    if (z->contains(p))
      return true;
  return false;
}

bool CompositeZoR::coveredBy(const Point &center, float radius) const {
  for (const auto &z : components)
    if (z->coveredBy(center, radius))
      return true;

  return false;
}

float CompositeZoR::distanceToBoundary(const Point &p) const {
  float minDist = INF;
  for (const auto &z : components)
    minDist = std::min(minDist, z->distanceToBoundary(p));
  return minDist;
}

void CompositeZoR::serialize(std::vector<uint8_t> &buf) const {
  uint8_t tag = getType();
  write(buf, tag);
  uint8_t count = components.size();
  write(buf, count);
  for (const auto &z : components) {
    z->serialize(buf);
  }
}

std::unique_ptr<CompositeZoR>
CompositeZoR::deserialize(const std::vector<uint8_t> &buf, size_t &offset) {
  std::vector<std::unique_ptr<ZoR>> components;

  uint8_t count = read<uint8_t>(buf, offset);
  for (uint8_t i = 0; i < count; ++i) {
    uint8_t tag = read<uint8_t>(buf, offset);

    switch (tag) {
      case NEIGHBOR:
        components.emplace_back(NeighborZoR::deserialize(buf, offset));
        break;
      case CIRCLE:
        components.emplace_back(CircleZoR::deserialize(buf, offset));
        break;
      case POLYGON:
        components.emplace_back(PolygonZoR::deserialize(buf, offset));
        break;
      case COMPOSITE:
        components.emplace_back(CompositeZoR::deserialize(buf, offset));
        break;
      default:
        NS_FATAL_ERROR("Unknown ZoRType=" << (int)tag);
        break;
    }
  }
  return std::make_unique<CompositeZoR>(std::move(components));
}

std::unique_ptr<ZoR> ZoR::deserialize(const std::vector<uint8_t> &buf) {
  size_t offset = 0;
  uint8_t tag = read<uint8_t>(buf, offset);

  switch (tag) {
    case NEIGHBOR:
      return NeighborZoR::deserialize(buf, offset);
    case CIRCLE:
      return CircleZoR::deserialize(buf, offset);
    case POLYGON:
      return PolygonZoR::deserialize(buf, offset);
    case COMPOSITE:
      return CompositeZoR::deserialize(buf, offset);
    default:
      return nullptr;
  }
}

std::vector<uint8_t> ZoR::serialize(const ZoR &zor) {
  std::vector<uint8_t> data;
  zor.serialize(data);
  return data;
}

} // namespace caf
} // namespace ns3