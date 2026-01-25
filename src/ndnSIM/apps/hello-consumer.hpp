#ifndef HELLO_CONSUMER_HPP
#define HELLO_CONSUMER_HPP

#include "ns3/ndnSIM/apps/alert-consumer.hpp"

namespace ns3 {
  
namespace ndn {
class HelloConsumer : public AlertConsumer {
public:
  static TypeId 
  GetTypeId(void);

  HelloConsumer();

  protected:
  virtual void
  doReceive(shared_ptr<const ndn::Data> data) override;
};

} // namespace ndn
} // namespace ns3
 

#endif // HELLO_CONSUMER_HPP