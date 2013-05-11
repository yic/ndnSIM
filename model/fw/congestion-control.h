//congestion-control.h

#ifndef CONGESTION_CONTROL_H
#define CONGESTION_CONTROL_H

#include "ns3/log.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-l3-protocol.h"
#include "nacks.h"
#include "per-out-face-limits.h"
#include "best-route.h"
#include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"

namespace ns3 {
namespace ndn {
namespace fw {

typedef PerOutFaceLimits<BestRoute> BaseStrategy;

class CongestionControlStrategy:
    public BaseStrategy
{
public:
  static TypeId GetTypeId();

  static std::string GetLogName();

  CongestionControlStrategy();

  virtual void OnInterest(Ptr<Face> face,
    Ptr<const InterestHeader> header,
    Ptr<const Packet> origPacket);

  virtual void DidSendOutInterest(Ptr<Face> inFace,
    Ptr<Face> outFace,
    Ptr<const Interest> header,
    Ptr<const Packet> origPacket,
    Ptr<pit::Entry> pitEntry);

  virtual void OnData(Ptr<Face> face,
    Ptr<const ContentObjectHeader> header,
    Ptr<Packet> payload,
    Ptr<const Packet> origPacket);

protected:
  virtual void OnNack(Ptr<Face> inFace,
    Ptr<const InterestHeader> header,
    Ptr<const Packet> origPacket);

protected:
  static LogComponent g_log;
  Ptr<RttEstimator> m_rtt;
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // CONGESTION_CONTROL_H
