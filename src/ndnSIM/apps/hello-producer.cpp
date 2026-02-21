#include "ns3/log.h"

#include "hello-producer.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.HelloProducer");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(HelloProducer);

TypeId
HelloProducer::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::ndn::HelloProducer")
          .SetGroupName("Caf")
          .SetParent<AlertProducerCbr>()
          .AddConstructor<HelloProducer>();
  return tid;
}

HelloProducer::HelloProducer()
{}

std::shared_ptr<Data>
HelloProducer::AlertSupplier()
{
  Name dataName(m_prefix);
  dataName.appendSequenceNumber(m_seq++);

  auto data = std::make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(time::milliseconds(m_freshness.GetMilliSeconds()));
  data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));

  NS_LOG_LOGIC("Send hello: " << data->getName());
  return data;
}

} // namespace ndn
} // namespace ns3