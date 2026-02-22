#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"

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
          .AddConstructor<HelloProducer>()
          .AddAttribute("Prefix", "Prefix to use for alerts",
                        StringValue("/alert"),
                        MakeNameAccessor(&HelloProducer::m_prefix),
                        MakeNameChecker())
          .AddAttribute("PayloadSize", "Virtual payload size for Content packets",
                        UintegerValue(1024),
                        MakeUintegerAccessor(&HelloProducer::m_virtualPayloadSize),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                        StringValue("0s"),
                        MakeTimeAccessor(&HelloProducer::m_freshness),
                        MakeTimeChecker());
  return tid;
}

HelloProducer::HelloProducer()
  : m_seq(0)
{}

std::shared_ptr<Data>
HelloProducer::AlertSupplier()
{
  Name dataName(m_prefix);
  dataName.appendSequenceNumber(m_seq++);
  dataName.appendZoR(caf::NeighborZoR());

  auto data = std::make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(time::milliseconds(m_freshness.GetMilliSeconds()));
  data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));

  NS_LOG_LOGIC("Send hello: " << data->getName());
  return data;
}

} // namespace ndn
} // namespace ns3