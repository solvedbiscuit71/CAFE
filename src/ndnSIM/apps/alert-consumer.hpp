#ifndef ALERT_CONSUMER_HPP
#define ALERT_CONSUMER_HPP

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ptr.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {
  
namespace ndn {
class AlertConsumer : public App {
public:
  static TypeId 
  GetTypeId(void);

  AlertConsumer();

  protected:
  virtual void 
  StartApplication() override;

  virtual void 
  StopApplication() override;
  
  virtual void 
  OnData(shared_ptr<const ndn::Data> data) override; 
  
  virtual void
  doReceive(shared_ptr<const ndn::Data> data);

  private:
  void 
  RegisterInterest();

  Ptr<UniformRandomVariable> m_rand;
  Name m_prefix;
  Time m_interestLifeTime;
};

} // namespace ndn
} // namespace ns3
 

#endif // ALERT_CONSUMER_HPP