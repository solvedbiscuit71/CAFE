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

#ifndef CAF_ZOR_HPP
#define CAF_ZOR_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <sys/types.h>
#include <vector>

namespace ns3 {
namespace caf {

constexpr float INF = std::numeric_limits<float>::max();

/**
 * \class Point is a 2 dimensional vector
 */
struct Point {
  float x, y;

  static size_t size() { return 2 * sizeof(float); }

  float distanceFromPoint(const Point &other) const;

  float distanceFromLine(const Point &a, const Point &b) const;
};

template <typename T> void write(std::vector<uint8_t> &buf, const T &value) {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(&value);
  for (size_t i = 0; i < sizeof(T); i++)
    buf.push_back(*(p + i));
}

template <typename T> T read(const std::vector<uint8_t> &buf, size_t &offset) {
  T value;
  std::memcpy(&value, buf.data() + offset, sizeof(T));
  offset += sizeof(T);
  return value;
}

enum ZoRType : uint8_t {
  CIRCLE = 0b00000001,
  POLYGON = 0b00000010,
  COMPOSITE = 0b00000100,
};

class ZoR {
public:
  virtual ZoRType getType() const = 0;
  virtual size_t size() const = 0;

  virtual bool contains(const Point &) const = 0;
  virtual bool coveredBy(const Point &, float) const = 0;
  virtual float distanceToBoundary(const Point &) const = 0;
  virtual void serialize(std::vector<uint8_t> &) const = 0;
  virtual ~ZoR() = default;

  static std::vector<uint8_t> serialize(const ZoR &);
  static std::unique_ptr<ZoR> deserialize(const std::vector<uint8_t> &);
};

class CircleZoR : public ZoR {
  Point center;
  float radius;

public:
  CircleZoR(Point c, float r) : center(c), radius(r) {}

  ZoRType getType() const override { return CIRCLE; }
  size_t size() const override;

  bool contains(const Point &p) const override;
  bool coveredBy(const Point &center, float radius) const override;
  float distanceToBoundary(const Point &p) const override;

  void serialize(std::vector<uint8_t> &buf) const override;
  static std::unique_ptr<CircleZoR> deserialize(const std::vector<uint8_t> &buf,
                                                size_t &offset);
};

class PolygonZoR : public ZoR {
  std::vector<Point> vertices;

public:
  // copy constructor
  PolygonZoR(const std::vector<Point> &v);

  // move constructor
  PolygonZoR(std::vector<Point> &&v);

  ZoRType getType() const override { return POLYGON; }
  size_t size() const override;

  bool contains(const Point &p) const override;
  bool coveredBy(const Point &center, float radius) const override;
  float distanceToBoundary(const Point &p) const override;

  void serialize(std::vector<uint8_t> &buf) const override;
  static std::unique_ptr<PolygonZoR>
  deserialize(const std::vector<uint8_t> &buf, size_t &offset);
};


class CompositeZoR : public ZoR {
  std::vector<std::unique_ptr<ZoR>> components;

public:
  // default constructor
  CompositeZoR() : components() {}

  // move constructor
  CompositeZoR(std::vector<std::unique_ptr<ZoR>> &&components);

  ZoRType getType() const override { return COMPOSITE; }
  size_t size() const override;

  bool append(std::unique_ptr<ZoR> zor);
  bool contains(const Point &p) const override;
  bool coveredBy(const Point &center, float radius) const override;
  float distanceToBoundary(const Point &p) const override;

  void serialize(std::vector<uint8_t> &buf) const override;
  static std::unique_ptr<CompositeZoR>
  deserialize(const std::vector<uint8_t> &buf, size_t &offset);
};

} // namespace caf
} // namespace ns3

#endif // CAF_ROUTING_HPP
