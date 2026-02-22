#include "alert-producer-cbr.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.AlertProducerCbr");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(AlertProducerCbr);

TypeId AlertProducerCbr::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::ndn::AlertProducerCbr")
          .SetGroupName("Caf")
          .SetParent<AlertProducer>()
          .AddConstructor<AlertProducerCbr>()
          .AddAttribute("Interval", "Interval between alerts",
                        StringValue("1s"),
                        MakeTimeAccessor(&AlertProducerCbr::m_interval),
                        MakeTimeChecker())
          .AddAttribute("EnableJitter", "Add jitter to prevent hidden terminal problem",
                        BooleanValue(false),
                        MakeBooleanAccessor(&AlertProducerCbr::m_jitter),
                        MakeBooleanChecker());
  return tid;
}

AlertProducerCbr::AlertProducerCbr() 
  : m_rand(CreateObject<NormalRandomVariable>())
{
  m_rand->SetAttribute("Mean", DoubleValue(0.0));
  m_rand->SetAttribute("Variance", DoubleValue(0.00001)); // std = 1ms = 0.001s
}

void
AlertProducerCbr::StartApplication() {
  AlertProducer::StartApplication();
  m_sendEvent =
      Simulator::Schedule(Seconds(0.0), &AlertProducerCbr::SendAlert, this);
}

void
AlertProducerCbr::StopApplication() {
  Simulator::Cancel(m_sendEvent);
  AlertProducer::StopApplication();
}

void
AlertProducerCbr::SendAlert() {
  doSend();

  Time jitter = Seconds(0);
  if (m_jitter) {
    jitter = Seconds(m_rand->GetValue());
  }
  m_sendEvent =
      Simulator::Schedule(m_interval + jitter, &AlertProducerCbr::SendAlert, this);
}

} // namespace ndn
} // namespace ns3