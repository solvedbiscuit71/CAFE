#include "model/caf-context.hpp"
#include "model/caf-zor.hpp"

#include "ndn-cxx/name.hpp"
#include <memory>

#include "test.h"

std::vector<uint8_t>
sampleZoR ()
{
  using namespace ns3::caf;
  CompositeZoR zor;

  std::unique_ptr<CompositeZoR> subZoR = std::make_unique<CompositeZoR> ();
  subZoR->append (std::make_unique<PolygonZoR> (
      std::vector<Point>{{200, 5}, {200, -5}, {-200, -5}, {-200, 5}}));
  subZoR->append (std::make_unique<PolygonZoR> (
      std::vector<Point>{{5, 200}, {5, -200}, {-5, -200}, {-5, 200}}));

  zor.append (std::make_unique<CircleZoR> (Point{0, 0}, 50));
  zor.append (std::move (subZoR));

  return ZoR::serialize (zor);
}

void
ZoRTest ()
{
  using namespace ns3::caf;

  std::unique_ptr<ZoR> zor = ZoR::deserialize (sampleZoR ());

  ndn::Name name ("/alert");
  name.appendSequenceNumber (1);
  name.appendZoR (*zor);

  std::cout << name << std::endl;
  std::cout << name.size () << std::endl;

  auto comp = name.get (name.size () - 1);
  if (comp.isZoR ()) {
    std::unique_ptr<ZoR> restoredZoR = comp.toZoR();
    
    float radius = 50.0f;

    Point rsu1{0, 0};
    Point rsu2{200, 0};
    Point rsu3{300, 0};

    std::cout << restoredZoR->coveredBy(rsu1, radius) << std::endl; // true
    std::cout << restoredZoR->coveredBy(rsu2, radius) << std::endl; // true
    std::cout << restoredZoR->coveredBy(rsu3, radius) << std::endl; // false

    Point veh1{420,2.5};
    Point veh2{450,3.5};
    
    std::cout << restoredZoR->distanceToBoundary(veh1) << ' '
              << restoredZoR->distanceToBoundary(veh2) << std::endl; // veh1 < veh2
  }
}