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
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

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
        .AddAttribute ("MinTh",
                "Minimum average length threshold in packets/bytes",
                DoubleValue(5.0),
                MakeDoubleAccessor(&CongestionControlStrategy::m_minTh),
                MakeDoubleChecker<double>())
        .AddAttribute ("MaxTh",
                "Maximum average length threshold in packets/bytes",
                DoubleValue(15.0),
                MakeDoubleAccessor(&CongestionControlStrategy::m_maxTh),
                MakeDoubleChecker<double>())
        .AddAttribute ("MaxP",
                "The maximum probability of dropping a packet",
                DoubleValue(0.02),
                MakeDoubleAccessor(&CongestionControlStrategy::m_maxP),
                MakeDoubleChecker<double> ())
        ;
    return tid;
}

CongestionControlStrategy::CongestionControlStrategy()
    : m_count(0)
    , m_wasBeyondMinTh(false)
{
    m_rtt = CreateObject<RttMeanDeviation>();
    m_random = CreateObject<UniformRandomVariable>();
}

bool CongestionControlStrategy::EarlyNack(Ptr<Face> face)
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

                        double u = m_random->GetValue();
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

void CongestionControlStrategy::OnInterest(Ptr<Face> face,
        Ptr<const InterestHeader> header,
        Ptr<const Packet> origPacket)
{
    if (EarlyNack(face)) {
        if (m_nacksEnabled) {
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

            face->Send (packet->Copy ());
        }
    }
    else {
        BaseStrategy::OnInterest(face, header, origPacket);
    }
}

void CongestionControlStrategy::DidSendOutInterest(Ptr<Face> inFace,
    Ptr<Face> outFace,
    Ptr<const Interest> header,
    Ptr<const Packet> origPacket,
    Ptr<pit::Entry> pitEntry)
{
    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace>(outFace);
    if (netDeviceFace) {
        uint32_t seq = boost::lexical_cast<uint32_t>(header->GetName().GetLastComponent());
        m_rtt->SentSeq(SequenceNumber32(seq), 1);
    }

    BaseStrategy::DidSendOutInterest(inFace, outFace, header, origPacket, pitEntry);
}

void CongestionControlStrategy::OnData(Ptr<Face> face,
        Ptr<const ContentObjectHeader> header,
        Ptr<Packet> payload,
        Ptr<const Packet> origPacket)
{
    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace>(face);
    if (netDeviceFace) {
        Ptr<Limits> faceLimits = face->GetObject<Limits>();

        uint32_t seq = boost::lexical_cast<uint32_t>(header->GetName().GetLastComponent());
        m_rtt->AckSeq(SequenceNumber32(seq));
        faceLimits->SetLimits(faceLimits->GetMaxRate(), m_rtt->GetCurrentEstimate().ToDouble(Time::S));

        double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() + 1.0);
        faceLimits->UpdateCurrentLimit(newLimit);
    }

  BaseStrategy::OnData(face, header, payload, origPacket);
}

void CongestionControlStrategy::OnNack(Ptr<Face> inFace,
        Ptr<const InterestHeader> header,
        Ptr<const Packet> origPacket)
{
    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace>(inFace);
    if (netDeviceFace) {
        Ptr<Limits> faceLimits = inFace->GetObject<Limits>();
        double newLimit = std::max(0.0, faceLimits->GetCurrentLimit() - 1.0);
        faceLimits->UpdateCurrentLimit(newLimit);
    }

    BaseStrategy::OnNack(inFace, header, origPacket);
}

} // namespace fw
} // namespace ndn
} // namespace ns3
