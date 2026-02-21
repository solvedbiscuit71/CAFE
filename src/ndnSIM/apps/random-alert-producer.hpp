#ifndef RANDOM_ALERT_PRODUCER_HPP
#define RANDOM_ALERT_PRODUCER_HPP

#include "ns3/ptr.h"
#include "ns3/event-id.h"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/random-variable-stream.h"

#include "ndn-app.hpp"
#include "model/caf-zor.hpp"

#include "alert-producer.hpp"

namespace ns3 {
  
namespace ndn {
class RandomAlertProducer : public AlertProducer {
public:
  static TypeId 
  GetTypeId(void);

  RandomAlertProducer();
  
  void
  SetZoR(std::shared_ptr<caf::ZoR> zor) { m_zor = zor; } 
  
private:
  EventId m_sendEvent;
  Ptr<UniformRandomVariable> m_rand;
  uint32_t m_seq;
  std::shared_ptr<caf::ZoR> m_zor;

  void 
  SendAlert();

protected:
  virtual void 
  StartApplication() override;

  virtual void 
  StopApplication() override;
  
  virtual std::shared_ptr<Data>
  AlertSupplier() override;

  Name m_prefix;
  Time m_interval;
  double m_threshold;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
};

} // namespace ndn
} // namespace ns3
 

#endif // ALERT_PRODUCER_CBR_HPP