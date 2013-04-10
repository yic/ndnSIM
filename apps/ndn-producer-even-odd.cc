#include "ndn-producer-even-odd.h"
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

NS_LOG_COMPONENT_DEFINE ("ndn.EvenOddProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (EvenOddProducer);
    
TypeId
EvenOddProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::EvenOddProducer")
    .SetGroupName ("Ndn")
    .SetParent<Producer> ()
    .AddConstructor<EvenOddProducer> ()
    .AddAttribute ("AcceptEven", "Whether or not to accept even chunks exclusively",
                   BooleanValue (false),
                   MakeBooleanAccessor (&EvenOddProducer::m_acceptEven),
                   MakeBooleanChecker ())

    ;
        
  return tid;
}
    
EvenOddProducer::EvenOddProducer ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}

void
EvenOddProducer::OnInterest (const Ptr<const Interest> &interest, Ptr<Packet> origPacket)
{

  if((atoi((interest->GetName().GetLastComponent ()).c_str()) % 2 != 1 && m_acceptEven) ||
     (atoi((interest->GetName().GetLastComponent ()).c_str()) % 2 == 1 && !m_acceptEven)){
    Producer::OnInterest (interest, origPacket);
  }
}

} // namespace ndn
} // namespace ns3
