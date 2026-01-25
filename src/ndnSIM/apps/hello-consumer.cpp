#include "hello-consumer.hpp"
#include "src/ndnSIM/apps/alert-consumer.hpp"

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
  NS_LOG_INFO("Received hello: " << data->getName());
}

} // namespace ndn

} // namespace ns3