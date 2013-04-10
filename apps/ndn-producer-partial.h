#ifndef NDN_PRODUCER_PARTIAL_H
#define NDN_PRODUCER_PARTIAL_H

#include "ndn-app.h"
#include "ndn-producer.h"

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-content-object.h"

namespace ns3 {
namespace ndn {

class PartialProducer : public Producer
{
public: 
  static TypeId
  GetTypeId (void);
        
  PartialProducer ();

  void OnInterest (const Ptr<const Interest> &interest, Ptr<Packet> packet);

private:
  uint32_t m_dividend;
  uint32_t m_remainder;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_PARTIAL_H
