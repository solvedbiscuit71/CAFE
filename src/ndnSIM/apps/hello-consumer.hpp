#ifndef HELLO_CONSUMER_HPP
#define HELLO_CONSUMER_HPP

#include "ns3/ndnSIM/apps/alert-consumer.hpp"
#include "ns3/nstime.h"
#include "model/caf-context.hpp"

namespace ns3 {
  
namespace ndn {
class HelloConsumer : public AlertConsumer {
public:
  static TypeId 
  GetTypeId(void);
  
  HelloConsumer() 
    : m_lastReceived(), m_ctx(nullptr) {};

protected:
  virtual void
  StartApplication() override; 

  virtual void
  doReceive(shared_ptr<const ndn::Data> data) override;

  virtual bool
  handleTimeout() override;
  
private:
  Time m_lastReceived;
  Ptr<caf::Context> m_ctx;
};

} // namespace ndn
} // namespace ns3
 

#endif // HELLO_CONSUMER_HPP