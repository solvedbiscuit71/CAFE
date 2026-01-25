#ifndef HELLO_PRODUCER_HPP
#define HELLO_PRODUCER_HPP

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "src/ndnSIM/apps/alert-producer-cbr.hpp"

namespace ns3 {
  
namespace ndn {
class HelloProducer : public AlertProducerCbr {
public:
  static TypeId 
  GetTypeId(void);

  HelloProducer();

protected:
  virtual void
  doSend() override;
};

} // namespace ndn
} // namespace ns3
 

#endif // HELLO_PRODUCER_HPP