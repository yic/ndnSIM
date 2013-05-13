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

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ndn-drop-tail-queue.h"

NS_LOG_COMPONENT_DEFINE ("ndn.DropTailQueue");

namespace ns3 {

namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (NdnDropTailQueue);

TypeId NdnDropTailQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::ndn::DropTailQueue")
    .SetParent<Queue> ()
    .AddConstructor<NdnDropTailQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&NdnDropTailQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this NdnDropTailQueue.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&NdnDropTailQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this NdnDropTailQueue.",
                   UintegerValue (100 * 65535),
                   MakeUintegerAccessor (&NdnDropTailQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QW",
                   "Queue weight related to the exponential weighted moving average (EWMA)",
                   DoubleValue (0.002),
                   MakeDoubleAccessor (&NdnDropTailQueue::m_qW),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("LinkBandwidth",
                   "The RED link bandwidth",
                   DataRateValue (DataRate ("1.5Mbps")),
                   MakeDataRateAccessor (&NdnDropTailQueue::m_linkBandwidth),
                   MakeDataRateChecker ())
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (500),
                   MakeUintegerAccessor (&NdnDropTailQueue::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

NdnDropTailQueue::NdnDropTailQueue () :
  Queue (),
  m_packets (),
  m_bytesInQueue (0),
  m_hasStarted (false)
{
  NS_LOG_FUNCTION (this);
}

NdnDropTailQueue::~NdnDropTailQueue ()
{
  NS_LOG_FUNCTION (this);
}

void
NdnDropTailQueue::SetMode (NdnDropTailQueue::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

NdnDropTailQueue::QueueMode
NdnDropTailQueue::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

void NdnDropTailQueue::InitializeParams(void)
{
  m_qAvg = 0.0;
  m_idle = true;
  m_idleTime = NanoSeconds (0);
  m_ptc = m_linkBandwidth.GetBitRate () / (8.0 * m_meanPktSize);

  if (m_qW == 0.0)
  {
      m_qW = 1.0 - std::exp (-10.0 / m_ptc);
  }
}

bool 
NdnDropTailQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  if (!m_hasStarted) {
      InitializeParams();
      m_hasStarted = true;
  }

  uint32_t nQueued = 0;

  if (GetMode () == QUEUE_MODE_BYTES)
  {
      nQueued = m_bytesInQueue;
  }
  else if (GetMode () == QUEUE_MODE_PACKETS)
  {
      nQueued = m_packets.size ();
  }

  // simulate number of packets arrival during idle period
  uint32_t m = 0;

  if (m_idle)
  {
      Time now = Simulator::Now ();
      m = uint32_t (m_ptc * (now - m_idleTime).GetSeconds ());
      m_idle = false;
  }

  m_qAvg = Estimator (nQueued, m + 1, m_qAvg, m_qW);

  if (m_mode == QUEUE_MODE_PACKETS && (m_packets.size () >= m_maxPackets))
    {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      Drop (p);
      return false;
    }

  if (m_mode == QUEUE_MODE_BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
    {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      Drop (p);
      return false;
    }

  m_bytesInQueue += p->GetSize ();
  m_packets.push (p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

Ptr<Packet>
NdnDropTailQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      m_idle = true;
      m_idleTime = Simulator::Now ();

      return 0;
    }

  NS_ASSERT(!m_idle);
  Ptr<Packet> p = m_packets.front ();
  m_packets.pop ();
  m_bytesInQueue -= p->GetSize ();

  NS_LOG_LOGIC ("Popped " << p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

Ptr<const Packet>
NdnDropTailQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

double NdnDropTailQueue::GetAverageQueueLength(void)
{
    return m_qAvg;
}

uint32_t NdnDropTailQueue::GetQueueLength(void)
{
    if (GetMode () == QUEUE_MODE_BYTES)
    {
        return m_bytesInQueue;
    }
    else if (GetMode () == QUEUE_MODE_PACKETS)
    {
        return m_packets.size ();
    }
    else
    {
        NS_ABORT_MSG ("Unknown mode.");
    }
}

double NdnDropTailQueue::Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW)
{
    NS_LOG_FUNCTION (this << nQueued << m << qAvg << qW);

    double newAve = qAvg;

    while (--m >= 1)
    {
        newAve *= 1.0 - qW;
    }

    newAve *= 1.0 - qW;
    newAve += qW * nQueued;

    return newAve;
}

} // namespace ndn

} // namespace ns3

