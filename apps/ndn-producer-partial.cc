#include "ndn-producer-partial.h"
#include "ns3/log.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.PartialProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (PartialProducer);
    
TypeId
PartialProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::PartialProducer")
    .SetGroupName ("Ndn")
    .SetParent<Producer> ()
    .AddConstructor<PartialProducer> ()
    .AddAttribute ("Dividend", "Dividend",
                   UintegerValue (1),
                   MakeUintegerAccessor (&PartialProducer::m_dividend),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Remainder", "Remainder",
                   UintegerValue (0),
                   MakeUintegerAccessor (&PartialProducer::m_remainder),
                   MakeUintegerChecker<uint32_t> ())
    ;
        
  return tid;
}
    
PartialProducer::PartialProducer ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}

void
PartialProducer::OnInterest (const Ptr<const Interest> &interest, Ptr<Packet> origPacket)
{

  if (atoi((interest->GetName().GetLastComponent ()).c_str()) % m_dividend == m_remainder) {
    Producer::OnInterest (interest, origPacket);
  }
}

} // namespace ndn
} // namespace ns3
