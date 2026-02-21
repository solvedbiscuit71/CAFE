#ifndef ALERT_PRODUCER_HPP
#define ALERT_PRODUCER_HPP

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

namespace ns3 {
  
namespace ndn {
class AlertProducer : public App {
public:
  static TypeId 
  GetTypeId(void);

  AlertProducer() = default;

protected:
  virtual void 
  StartApplication() override;

  virtual void 
  StopApplication() override;
  
  virtual void
  doSend() final;
  
  virtual std::shared_ptr<Data>
  AlertSupplier();

  uint32_t m_signature;
  Name m_keyLocator;
};

} // namespace ndn
} // namespace ns3
 

#endif // ALERT_PRODUCER_CBR_HPP