#ifndef CAF_STACK_HELPER_HPP
#define CAF_STACK_HELPER_HPP

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/model/caf-context.hpp"
#include "ns3/node-container.h"
#include "ndn-stack-helper.hpp"
#include "ns3/nstime.h"


namespace ns3 {
namespace caf {

/**
 * @ingroup caf
 * @defgroup caf-helpers Helpers
 */
/**
 * @ingroup caf-helpers
 * @brief Helper class to install CAF stack and configure its parameters
 */
class StackHelper : boost::noncopyable {
public:
StackHelper();
virtual ~StackHelper();

bool getEnableHello() { return m_enableHello; };
void setEnableHello(bool enableHello) { m_enableHello = enableHello; };

Time getStartTime() { return m_startTime; };
void setStartTime(Time startTime) { m_startTime = startTime; };

void
Install(NodeContainer nodes,
        bool SetDefaultRoutes = false,
        size_t maxCsSize = 100);

private:
void
SetupRSU(Ptr<Node> node, Ptr<Context> ctx);

void
SetupVehicle(Ptr<Node> node, Ptr<Context> ctx);

void
SetupBackbone(Ptr<Node> node, Ptr<Context> ctx);

ndn::StackHelper m_helper;
bool m_enableHello;
Time m_startTime;

};

} // namespace caf
} // namespace ns3

#endif // CAF_STACK_HELPER_HPP
