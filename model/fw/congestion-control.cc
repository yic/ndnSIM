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
#include "ns3/ndn-forwarding-strategy.h"

#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>

#include "ns3/ndn-net-device-face.h"
#include "ns3/ndn-drop-tail-queue.h"
#include "ns3/point-to-point-net-device.h"

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED(CongestionControlStrategy);

LogComponent CongestionControlStrategy::g_log = LogComponent(CongestionControlStrategy::GetLogName().c_str());
    
std::string CongestionControlStrategy::GetLogName()
{
  return BaseStrategy::GetLogName()+".CongestionControlStrategy";
}
    
TypeId CongestionControlStrategy::GetTypeId(void)
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
  m_rtt = CreateObject<RttMeanDeviation>();
}


void CongestionControlStrategy::OnInterest(Ptr<Face> face,
        Ptr<const InterestHeader> header,
        Ptr<const Packet> origPacket)
{
    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace>(face);
    if (netDeviceFace) {
        Ptr<PointToPointNetDevice> p2pNetDevice = DynamicCast<PointToPointNetDevice>(netDeviceFace->GetNetDevice());
        if (p2pNetDevice) {
            Ptr<NdnDropTailQueue> queue = DynamicCast<NdnDropTailQueue>(p2pNetDevice->GetQueue());
            if (queue) {
                std::cout << queue->GetAverageQueueLength() << std::endl;
            }
        }
    }

    BaseStrategy::OnInterest(face, header, origPacket);
}

void CongestionControlStrategy::DidSendOutInterest(Ptr<Face> inFace,
    Ptr<Face> outFace,
    Ptr<const Interest> header,
    Ptr<const Packet> origPacket,
    Ptr<pit::Entry> pitEntry)
{
  uint32_t seq = boost::lexical_cast<uint32_t>(header->GetName().GetLastComponent());
  m_rtt->SentSeq(SequenceNumber32(seq), 1);

  BaseStrategy::DidSendOutInterest(inFace, outFace, header, origPacket, pitEntry);
}

void CongestionControlStrategy::OnData(Ptr<Face> face,
        Ptr<const ContentObjectHeader> header,
        Ptr<Packet> payload,
        Ptr<const Packet> origPacket)
{
  Ptr<Limits> faceLimits = face->GetObject<Limits>();
  double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() + 1.0);
  faceLimits->UpdateCurrentLimit(newLimit);

  uint32_t seq = boost::lexical_cast<uint32_t>(header->GetName().GetLastComponent());
  m_rtt->AckSeq(SequenceNumber32(seq));
  faceLimits->SetLimits(faceLimits->GetMaxRate(), m_rtt->GetCurrentEstimate().ToDouble(Time::S)); 

  BaseStrategy::OnData(face, header, payload, origPacket);
}

void CongestionControlStrategy::OnNack(Ptr<Face> inFace,
        Ptr<const InterestHeader> header,
        Ptr<const Packet> origPacket)
{
  Ptr<Limits> faceLimits = inFace->GetObject<Limits>();
  double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() - 1.0);
  faceLimits->UpdateCurrentLimit(newLimit);

  BaseStrategy::OnNack(inFace, header, origPacket);
}

} // namespace fw
} // namespace ndn
} // namespace ns3
