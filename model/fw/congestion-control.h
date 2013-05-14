//congestion-control.h

#ifndef CONGESTION_CONTROL_H
#define CONGESTION_CONTROL_H

#include "ns3/log.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-l3-protocol.h"
#include "ns3/random-variable-stream.h"
#include "nacks.h"
#include "per-out-face-limits.h"
#include "best-route.h"

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

protected:
  virtual void DidReceiveValidNack(Ptr<Face> inFace,
    uint32_t nackCode,
    Ptr<const Interest> header,
    Ptr<const Packet> origPacket,
    Ptr<pit::Entry> pitEntry);

  virtual void WillSatisfyPendingInterest(Ptr<Face> inFace,
    Ptr<pit::Entry> pitEntry);

protected:
  static LogComponent g_log;

private:
  double m_maxP;
  double m_minTh;
  double m_maxTh;
  uint32_t m_count;
  bool m_wasBeyondMinTh;
  bool m_earlyNackEnabled;
  bool m_dynamicLimitEnabled;
  Ptr<UniformRandomVariable> m_ranvar;

  bool EarlyNack(Ptr<Face> face);
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // CONGESTION_CONTROL_H
