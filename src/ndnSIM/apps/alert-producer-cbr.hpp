#ifndef ALERT_PRODUCER_CBR_HPP
#define ALERT_PRODUCER_CBR_HPP

#include "ns3/ptr.h"
#include "ns3/event-id.h"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/random-variable-stream.h"

#include "ndn-app.hpp"

#include "alert-producer.hpp"

namespace ns3 {
  
namespace ndn {
class AlertProducerCbr : public AlertProducer {
public:
  static TypeId 
  GetTypeId(void);

  AlertProducerCbr();

private:
  Ptr<NormalRandomVariable> m_rand;
  Time m_interval;
  bool m_jitter;
  EventId m_sendEvent;

  void 
  SendAlert();

protected:
  virtual void 
  StartApplication() override;

  virtual void 
  StopApplication() override;
  
  virtual std::shared_ptr<Data>
  AlertSupplier() override;

  uint32_t m_seq;
  Name m_prefix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
};

} // namespace ndn
} // namespace ns3
 

#endif // ALERT_PRODUCER_CBR_HPP