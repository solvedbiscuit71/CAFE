#include "hello-producer.hpp"
#include "alert-producer-cbr.hpp"

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

void
HelloProducer::doSend()
{
  Name dataName(m_prefix);
  dataName.appendSequenceNumber(m_seq++);

  auto data = std::make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(time::milliseconds(m_freshness.GetMilliSeconds()));

  data->setContent(make_shared<::ndn::Buffer>(m_virtualPayloadSize));

  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  data->setSignatureInfo(signatureInfo);

  ::ndn::EncodingEstimator estimator;
  ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
  encoder.appendVarNumber(m_signature);
  data->setSignatureValue(encoder.getBuffer());

  NS_LOG_INFO("Send hello: "  << data->getName());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data); // send to Forwarder
}

} // namespace ndn

} // namespace ns3