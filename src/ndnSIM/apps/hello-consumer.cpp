#include "hello-consumer.hpp"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE("ndn.HelloConsumer");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(HelloConsumer);

TypeId HelloConsumer::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::ndn::HelloConsumer")
          .SetGroupName("Caf")
          .SetParent<AlertConsumer>()
          .AddConstructor<HelloConsumer>();

  return tid;
}

HelloConsumer::HelloConsumer()
{ }

void
HelloConsumer::doReceive(shared_ptr<const ndn::Data> data)
{
  NS_LOG_INFO("Received message: " << data->getName());
  
  m_lastReceived = Simulator::Now();
}

bool
HelloConsumer::handleTimeout()
{
  NS_LOG_INFO("Timeout: last message received at " << m_lastReceived.GetSeconds());

  // re-inject the interest
  return true;
}

} // namespace ndn

} // namespace ns3