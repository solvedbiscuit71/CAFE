#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"

#include "random-alert-producer.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.RandomAlertProducer");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(RandomAlertProducer);

TypeId RandomAlertProducer::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::ndn::RandomAlertProducer")
          .SetGroupName("Caf")
          .SetParent<AlertProducer>()
          .AddConstructor<RandomAlertProducer>()
          .AddAttribute("Prefix", "Prefix to use for alerts",
                        StringValue("/alert"),
                        MakeNameAccessor(&RandomAlertProducer::m_prefix),
                        MakeNameChecker())
          .AddAttribute("Interval", "Interval between alerts",
                        StringValue("1s"),
                        MakeTimeAccessor(&RandomAlertProducer::m_interval),
                        MakeTimeChecker())
          .AddAttribute("Threshold", "Probability Threshold of sending an alert (0.0 to 1.0)",
                        DoubleValue(0.5),
                        MakeDoubleAccessor(&RandomAlertProducer::m_threshold),
                        MakeDoubleChecker<double>(0.0, 1.0))
          .AddAttribute("PayloadSize", "Virtual payload size for Content packets",
                        UintegerValue(1024),
                        MakeUintegerAccessor(&RandomAlertProducer::m_virtualPayloadSize),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                        StringValue("0s"),
                        MakeTimeAccessor(&RandomAlertProducer::m_freshness),
                        MakeTimeChecker());
  return tid;
}

RandomAlertProducer::RandomAlertProducer() 
  : m_rand(CreateObject<UniformRandomVariable>()), m_seq(0), m_zor(nullptr)
{
}

void
RandomAlertProducer::StartApplication() {
  AlertProducer::StartApplication();
  m_sendEvent = Simulator::Schedule(Seconds(0.0), &RandomAlertProducer::SendAlert, this);
}

void
RandomAlertProducer::StopApplication() {
  Simulator::Cancel(m_sendEvent);
  AlertProducer::StopApplication();
}

void
RandomAlertProducer::SendAlert() {
  double x = m_rand->GetValue();
  if (x < m_threshold) {
    doSend();
  }

  // if m_interval is zero then don't schedule
  if (!m_interval.IsZero()) {
    m_sendEvent = Simulator::Schedule(m_interval, &RandomAlertProducer::SendAlert, this);
  }
}

std::shared_ptr<Data>
RandomAlertProducer::AlertSupplier()
{
  Name dataName(m_prefix);
  dataName.appendSequenceNumber(m_seq++);
  if (m_zor) {
    dataName.appendZoR(*m_zor);
  }

  auto data = std::make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(time::milliseconds(m_freshness.GetMilliSeconds()));
  data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));

  NS_LOG_LOGIC("Send alert: " << data->getName());
  return data;
}

} // namespace ndn

} // namespace ns3