#include "model/ndn-common.hpp"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

#include "alert-producer.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.AlertProducer");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(AlertProducer);

TypeId AlertProducer::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::ndn::AlertProducer")
          .SetGroupName("Caf")
          .SetParent<App>()
          .AddConstructor<AlertProducer>()
          .AddAttribute("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                        UintegerValue(0),
                        MakeUintegerAccessor(&AlertProducer::m_signature),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                        NameValue(),
                        MakeNameAccessor(&AlertProducer::m_keyLocator),
                        MakeNameChecker());
  return tid;
}

void
AlertProducer::StartApplication() {
  NS_LOG_DEBUG("StartApplication()");
  App::StartApplication();
}

void
AlertProducer::StopApplication() {
  App::StopApplication();
}

shared_ptr<Data>
AlertProducer::AlertSupplier()
{
  NS_FATAL_ERROR("AlertSupplier() should be override by subclass");
  return nullptr;
}

void
AlertProducer::doSend()
{
  // Subclass should implment the alert supplier
  auto data = AlertSupplier();

  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  data->setSignatureInfo(signatureInfo);

  ::ndn::EncodingEstimator estimator;
  ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
  encoder.appendVarNumber(m_signature);
  data->setSignatureValue(encoder.getBuffer());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data); // send to Forwarder
}

} // namespace ndn

} // namespace ns3