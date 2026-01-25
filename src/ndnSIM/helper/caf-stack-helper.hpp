#ifndef CAF_STACK_HELPER_HPP
#define CAF_STACK_HELPER_HPP

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/model/caf-context.hpp"
#include "ns3/node-container.h"
#include "ndn-stack-helper.hpp"


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
class CafStackHelper : boost::noncopyable {
public:
CafStackHelper();
virtual ~CafStackHelper();

void
Install(NodeContainer nodes,
        bool SetDefaultRoutes = false,
        size_t maxCsSize = 100);

private:
void
SetupRSU(Ptr<Node> node);

void
SetupVehicle(Ptr<Node> node);

void
SetupBackbone(Ptr<Node> node);

ndn::StackHelper m_helper;

};

} // namespace caf
} // namespace ns3

#endif // CAF_STACK_HELPER_HPP
