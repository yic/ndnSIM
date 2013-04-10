#ifndef NDN_PRODUCER_EVEN_ODD_H
#define NDN_PRODUCER_EVEN_ODD_H

#include "ndn-app.h"
#include "ndn-producer.h"

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-content-object.h"

namespace ns3 {
namespace ndn {

class EvenOddProducer : public Producer
{
public: 
  static TypeId
  GetTypeId (void);
        
  EvenOddProducer ();

  void OnInterest (const Ptr<const Interest> &interest, Ptr<Packet> packet);

private:
  bool m_acceptEven;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_EVEN_ODD_H
