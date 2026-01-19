#include "alert-producer-cbr.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/utils/ndn-ns3-packet-tag.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.AlertProducerCbr");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(AlertProducerCbr);

TypeId AlertProducerCbr::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::ndn::AlertProducerCbr")
          .SetGroupName("Caf")
          .SetParent<App>()
          .AddConstructor<AlertProducerCbr>()
          .AddAttribute(
              "Prefix", "Prefix to use for alerts", StringValue("/alert"),
              MakeNameAccessor(&AlertProducerCbr::m_prefix), MakeNameChecker())
          .AddAttribute("Interval", "Interval between alerts",
                        TimeValue(Seconds(1.0)),
                        MakeTimeAccessor(&AlertProducerCbr::m_interval),
                        MakeTimeChecker())
          .AddAttribute(
              "PayloadSize", "Virtual payload size for Content packets",
              UintegerValue(1024),
              MakeUintegerAccessor(&AlertProducerCbr::m_virtualPayloadSize),
              MakeUintegerChecker<uint32_t>())
          .AddAttribute(
              "Freshness",
              "Freshness of data packets, if 0, then unlimited freshness",
              TimeValue(Seconds(0)),
              MakeTimeAccessor(&AlertProducerCbr::m_freshness),
              MakeTimeChecker())
          .AddAttribute("Signature",
                        "Fake signature, 0 valid signature (default), other "
                        "values application-specific",
                        UintegerValue(0),
                        MakeUintegerAccessor(&AlertProducerCbr::m_signature),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("KeyLocator",
                        "Name to be used for key locator.  If root, then key "
                        "locator is not used",
                        NameValue(),
                        MakeNameAccessor(&AlertProducerCbr::m_keyLocator),
                        MakeNameChecker());
  return tid;
}

AlertProducerCbr::AlertProducerCbr() : m_seq(0) {}

void AlertProducerCbr::StartApplication() {
  App::StartApplication();
  m_sendEvent =
      Simulator::Schedule(Seconds(0.0), &AlertProducerCbr::SendAlert, this);
}

void AlertProducerCbr::StopApplication() {
  Simulator::Cancel(m_sendEvent);
  App::StopApplication();
}

void AlertProducerCbr::SendAlert() {
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

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") pushing Alert: " << data->getName());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data); // send to Forwarder

  m_sendEvent =
      Simulator::Schedule(m_interval, &AlertProducerCbr::SendAlert, this);
}

} // namespace ndn

} // namespace ns3