#include "alert-consumer.hpp"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("ndn.AlertConsumer");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(AlertConsumer);

TypeId AlertConsumer::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::ndn::AlertConsumer")
          .SetGroupName("Caf")
          .SetParent<App>()
          .AddConstructor<AlertConsumer>()
          .AddAttribute("Prefix", "Prefix to use for alerts",
                        StringValue("/alert"),
                        MakeNameAccessor(&AlertConsumer::m_prefix),
                        MakeNameChecker())
          .AddAttribute("LifeTime", "LifeTime for interest packet",
                        StringValue("60s"),
                        MakeTimeAccessor(&AlertConsumer::m_interestLifeTime),
                        MakeTimeChecker());

  return tid;
}

AlertConsumer::AlertConsumer()
  : m_rand(CreateObject<UniformRandomVariable>())
{

}

AlertConsumer::~AlertConsumer()
{
  Simulator::Cancel(m_eventId);
}

void
AlertConsumer::StartApplication() {
  NS_LOG_DEBUG("StartApplication()");
  App::StartApplication();

  // Inject the first Interest to prime the PIT
  RegisterInterest();
}

void 
AlertConsumer::StopApplication() { App::StopApplication(); }

void
AlertConsumer::OnData(shared_ptr<const ndn::Data> data)
{
  doReceive(data);

  // Call trace source
  m_receivedDatas(data, this, m_face);

  // Because the previous PIT entry was satisfied and cleared,
  // we must immediately express interest again to stay "subscribed".
  RegisterInterest();
}

void
AlertConsumer::doReceive(shared_ptr<const ndn::Data> data)
{
  NS_LOG_INFO("Received alert: " << data->getName());
}

void 
AlertConsumer::RegisterInterest() {
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(m_prefix);
  interest->setCanBePrefix(true);
  interest->setMustBeFresh(false);

  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  NS_LOG_INFO("Inject interest: " << interest->getName());

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
  
  Simulator::Cancel(m_eventId);
  m_eventId = Simulator::Schedule(m_interestLifeTime, &AlertConsumer::OnTimeout, this);
}

void
AlertConsumer::OnTimeout() {
 if (handleTimeout()) {
   RegisterInterest();
 }
}

bool
AlertConsumer::handleTimeout() {
 NS_LOG_DEBUG("Interest timeout: " << m_prefix);
 
 // By default, all re-inject the interest
 return true;
}

} // namespace ndn

} // namespace ns3