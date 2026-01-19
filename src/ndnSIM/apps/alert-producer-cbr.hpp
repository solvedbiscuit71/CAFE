#ifndef ALERT_PRODUCER_CBR_HPP
#define ALERT_PRODUCER_CBR_HPP

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ptr.h"
#include "ns3/event-id.h"

namespace ns3 {
  
namespace ndn {
class AlertProducerCbr : public App {
public:
  static TypeId 
  GetTypeId(void);

  AlertProducerCbr();

  protected:
  virtual void 
  StartApplication() override;

  virtual void 
  StopApplication() override;

  private:
  void 
  SendAlert();

  Name m_prefix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  Time m_interval;
  EventId m_sendEvent;
  uint32_t m_seq;

  uint32_t m_signature;
  Name m_keyLocator;
};

} // namespace ndn
} // namespace ns3
 

#endif // ALERT_PRODUCER_CBR_HPP