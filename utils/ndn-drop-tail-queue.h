/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NDN_DROPTAIL_H
#define NDN_DROPTAIL_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

namespace ns3 {

namespace ndn {

class TraceContainer;

/**
 * \ingroup queue
 *
 * \brief A FIFO packet queue that drops tail-end packets on overflow
 */
class NdnDropTailQueue : public Queue {
public:
  static TypeId GetTypeId (void);
  /**
   * \brief NdnDropTailQueue Constructor
   *
   * Creates a droptail queue with a maximum size of 100 packets by default
   */
  NdnDropTailQueue ();

  virtual ~NdnDropTailQueue();

  /**
   * Set the operating mode of this device.
   *
   * \param mode The operating mode of this device.
   *
   */
  void SetMode (NdnDropTailQueue::QueueMode mode);

  /**
   * Get the encapsulation mode of this device.
   *
   * \returns The encapsulation mode of this device.
   */
  NdnDropTailQueue::QueueMode GetMode (void);

  double GetAverageQueueLength (void);
  uint32_t GetQueueLength (void);

private:
  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;

  std::queue<Ptr<Packet> > m_packets;
  uint32_t m_maxPackets;
  uint32_t m_maxBytes;
  uint32_t m_bytesInQueue;
  QueueMode m_mode;

private:
  bool m_hasStarted;
  double m_qAvg;
  bool m_idle;
  Time m_idleTime;
  double m_ptc;

  double m_qW;
  uint32_t m_meanPktSize;
  DataRate m_linkBandwidth;

  void InitializeParams(void);
  double Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW);
};

} // namespace ndn

} // namespace ns3

#endif /* NDN_DROPTAIL_H */
