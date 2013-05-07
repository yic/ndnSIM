//congestion-control.cc
#include "congestion-control.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-fib-entry.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"
#include "green-yellow-red.h"

#include "ns3/ndn-pit.h"
#include "ns3/ndn-content-object.h"
#include "ns3/ndn-content-store.h"

#include "ns3/assert.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/string.h"

#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include "ns3/ndn-forwarding-strategy.h"

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED(CongestionControlStrategy);

LogComponent CongestionControlStrategy::g_log = LogComponent(CongestionControlStrategy::GetLogName().c_str());
    
std::string
CongestionControlStrategy::GetLogName()
{
  return BaseStrategy::GetLogName()+".CongestionControlStrategy";
}
    
TypeId
CongestionControlStrategy::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::ndn::fw::BestRoute::PerOutFaceLimits::CongestionControlStrategy")
    .SetGroupName("Ndn")
    .SetParent<BaseStrategy>()
    .AddConstructor<CongestionControlStrategy>()
    ;
  return tid;
}

CongestionControlStrategy::CongestionControlStrategy()
{
}

void
CongestionControlStrategy::OnData(Ptr<Face> face,
          Ptr<const ContentObjectHeader> header,
          Ptr<Packet> payload,
          Ptr<const Packet> origPacket){
  Ptr<Limits> faceLimits = face->GetObject<Limits>();
  double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() + 1.0);
  faceLimits->UpdateCurrentLimit(newLimit);
  BaseStrategy::OnData(face, header, payload, origPacket);
}

void
CongestionControlStrategy::OnNack(Ptr<Face> inFace,
                        Ptr<const InterestHeader> header,
                        Ptr<const Packet> origPacket){
  Ptr<Limits> faceLimits = inFace->GetObject<Limits>();
  double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() - 1.0);
  faceLimits->UpdateCurrentLimit(newLimit);
  BaseStrategy::OnNack(inFace, header, origPacket);
}


} // namespace fw
} // namespace ndn
} // namespace ns3
