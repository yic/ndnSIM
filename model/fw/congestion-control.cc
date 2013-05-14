//congestion-control.cc
#include "best-route.h"
#include "flooding.h"
#include "smart-flooding.h"
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
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

namespace ns3 {
namespace ndn {
namespace fw {

extern template class PerOutFaceLimits<BestRoute>;
template class CongestionControl< PerOutFaceLimits<BestRoute> >;
typedef CongestionControl< PerOutFaceLimits<BestRoute> > CongestionControlPerOutFaceLimitsBestRoute;
NS_OBJECT_ENSURE_REGISTERED(CongestionControlPerOutFaceLimitsBestRoute);

extern template class PerOutFaceLimits<Flooding>;
template class CongestionControl< PerOutFaceLimits<Flooding> >;
typedef CongestionControl< PerOutFaceLimits<Flooding> > CongestionControlPerOutFaceLimitsFlooding;
NS_OBJECT_ENSURE_REGISTERED(CongestionControlPerOutFaceLimitsFlooding);

extern template class PerOutFaceLimits<SmartFlooding>;
template class CongestionControl< PerOutFaceLimits<SmartFlooding> >;
typedef CongestionControl< PerOutFaceLimits<SmartFlooding> > CongestionControlPerOutFaceLimitsSmartFlooding;
NS_OBJECT_ENSURE_REGISTERED(CongestionControlPerOutFaceLimitsSmartFlooding);

template<class Parent>
LogComponent CongestionControl<Parent>::g_log = LogComponent(CongestionControl<Parent>::GetLogName().c_str());
    
template<class Parent>
std::string CongestionControl<Parent>::GetLogName()
{
    return Parent::GetLogName()+".CongestionControl";
}
    
template<class Parent>
TypeId CongestionControl<Parent>::GetTypeId(void)
{
    static TypeId tid = TypeId((Parent::GetTypeId().GetName()+"::CongestionControl").c_str())
        .SetGroupName("Ndn")
        .template SetParent<Parent>()
        .template AddConstructor<CongestionControl>()
        .AddAttribute("MinTh",
                "Minimum average length threshold in packets/bytes",
                DoubleValue(5.0),
                MakeDoubleAccessor(&CongestionControl<Parent>::m_minTh),
                MakeDoubleChecker<double>())
        .AddAttribute("MaxTh",
                "Maximum average length threshold in packets/bytes",
                DoubleValue(15.0),
                MakeDoubleAccessor(&CongestionControl<Parent>::m_maxTh),
                MakeDoubleChecker<double>())
        .AddAttribute("MaxP",
                "The maximum probability of dropping a packet",
                DoubleValue(0.02),
                MakeDoubleAccessor(&CongestionControl<Parent>::m_maxP),
                MakeDoubleChecker<double>())
        .AddAttribute("EnableREN", "Enable random early NACK",
                BooleanValue(true),
                MakeBooleanAccessor(&CongestionControl<Parent>::m_earlyNackEnabled),
                MakeBooleanChecker())
        .AddAttribute("EnableDynamicLimit", "Enable dynamic Interest limit",
                BooleanValue(true),
                MakeBooleanAccessor(&CongestionControl<Parent>::m_dynamicLimitEnabled),
                MakeBooleanChecker())
        .AddAttribute("RttMultiplier",
                "RTT multiplier in calculating dynamic Interest limit",
                DoubleValue(1.0),
                MakeDoubleAccessor(&CongestionControl<Parent>::m_rttMultiplier),
                MakeDoubleChecker<double>())
        ;
    return tid;
}

template<class Parent>
CongestionControl<Parent>::CongestionControl()
    : m_count(0)
    , m_wasBeyondMinTh(false)
{
    m_ranvar = CreateObject<UniformRandomVariable>();
}

template<class Parent>
bool CongestionControl<Parent>::EarlyNack(Ptr<Face> face)
{
    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace>(face);
    if (netDeviceFace) {
        Ptr<PointToPointNetDevice> p2pNetDevice = DynamicCast<PointToPointNetDevice>(netDeviceFace->GetNetDevice());
        if (p2pNetDevice) {
            Ptr<NdnDropTailQueue> queue = DynamicCast<NdnDropTailQueue>(p2pNetDevice->GetQueue());
            if (queue) {
                double qAvg = queue->GetAverageQueueLength();

                if (qAvg >= m_minTh && queue->GetQueueLength() > 1) {
                    if (qAvg >= m_maxTh) {
                        //Definitely NACK
                        return true;
                    }
                    else if (!m_wasBeyondMinTh) {
                        m_count = 1;
                        m_wasBeyondMinTh = true;
                    }
                    else {
                        //NACK by probability
                        m_count ++;

                        double p = m_maxP * (qAvg - m_minTh) / (m_maxTh - m_minTh);
                        if (m_count * p < 1.0)
                            p /= 1.0 - m_count * p;
                        else
                            p = 1.0;

                        double u = m_ranvar->GetValue();
                        if (u < p) {
                            m_count = 0;
                            return true;
                        }
                    }
                }
                else {
                    m_wasBeyondMinTh = false;
                }
            }
        }
    }

    return false;
}

template<class Parent>
void CongestionControl<Parent>::OnInterest(Ptr<Face> face,
        Ptr<const InterestHeader> header,
        Ptr<const Packet> origPacket)
{
    if (m_earlyNackEnabled && EarlyNack(face)) {
        if (CongestionControl<Parent>::m_nacksEnabled) {
            NS_LOG_DEBUG("Sending early NACK");

            Ptr<Packet> packet = Create<Packet>();
            Ptr<Interest> nackHeader = Create<Interest>(*header);
            nackHeader->SetNack(Interest::NACK_CONGESTION);
            packet->AddHeader(*nackHeader);

            FwHopCountTag hopCountTag;
            if (origPacket->PeekPacketTag(hopCountTag)) {
                packet->AddPacketTag(hopCountTag);
            }
            else {
                NS_LOG_DEBUG("No FwHopCountTag tag associated with original Interest");
            }

            face->Send(packet->Copy());
        }
    }
    else {
        Parent::OnInterest(face, header, origPacket);
    }
}

template<class Parent>
void CongestionControl<Parent>::WillSatisfyPendingInterest(Ptr<Face> inFace,
    Ptr<pit::Entry> pitEntry)
{
    Parent::WillSatisfyPendingInterest(inFace, pitEntry);

    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace>(inFace);
    if (netDeviceFace) {
        Ptr<Limits> faceLimits = inFace->GetObject<Limits>();

        if (m_dynamicLimitEnabled) {
            faceLimits->SetLimits(faceLimits->GetMaxRate(), m_rttMultiplier * pitEntry->GetFibEntry()->GetFaceRtt(inFace).ToDouble(Time::S));
        }

        double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() + 1.0);
        faceLimits->UpdateCurrentLimit(newLimit);
    }
}

template<class Parent>
void CongestionControl<Parent>::DidReceiveValidNack(Ptr<Face> inFace,
    uint32_t nackCode,
    Ptr<const Interest> header,
    Ptr<const Packet> origPacket,
    Ptr<pit::Entry> pitEntry)
{
    Parent::DidReceiveValidNack(inFace, nackCode, header, origPacket, pitEntry);

    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace>(inFace);
    if (netDeviceFace) {
        Ptr<Limits> faceLimits = inFace->GetObject<Limits>();
        double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() - 1.0);
        faceLimits->UpdateCurrentLimit(newLimit);
    }
}

} // namespace fw
} // namespace ndn
} // namespace ns3
