
/**
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
 *
 * Author: Brett Levasseur
 */
/**
 *
 * This class implementes TCP CUBIC for ns-3 based on the class TcpSocketBase.
 * As packets are sent, arive or loss events occur the TcpSocketBase class is
 * called to process what should be done. Congestion control algorithms for TCP
 * extend from TcpSocketBase and override a set of methods to implement their
 * specific rules. Much of the code in this class was taken from the ns-2
 * and Linux Kernel 3.11 implementations to ensure this implementation would
 * be as realistic to what is actually implemented as possible. This code
 * did however need to be modified so that it would work within the setup of
 * TcpSocketBase.
 *
 */

#include "tcp-cubic.h"
#include <ns3/log.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/simulator.h>
#include <ns3/abort.h>
#include <ns3/node.h>

NS_LOG_COMPONENT_DEFINE ("TcpCubic");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(TcpCubic);

/**
 * Get the TypeId which allows users running the simulation to set configurable
 * parameters.
 */
TypeId TcpCubic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpCubic")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpCubic> ()
    .AddTraceSource ("CongestionWindowCount",
                     "Number of ACKs required to modify the congestion window.",
                     MakeTraceSourceAccessor (&TcpCubic::m_cWndCnt),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("EpochStart",
                     "The time of the last packet loss.",
                     MakeTraceSourceAccessor (&TcpCubic::m_epochStart),
                     "ns3::TracedValueCallback::Int64")
    .AddTraceSource ("CongestionWindowMax",
                     "The congestion window size before the last reduction.",
                     MakeTraceSourceAccessor (&TcpCubic::m_lastMax),
                     "ns3::TracedValueCallback::Unit32")
    .AddTraceSource ("CongestionOriginPoint",
                     "The starting size of the congestion window at the last window reduction.",
                     MakeTraceSourceAccessor (&TcpCubic::m_originPoint),
                     "ns3::TracedValueCallback::Unit32")
    .AddTraceSource ("ConcaveConvexChange",
                     "A conviencnce trace for checking when the algorithm switches from below to above the origin.",
                     MakeTraceSourceAccessor (&TcpCubic::m_belowOrigin),
                     "ns3::TracedValueCallback::Bool")
    .AddTraceSource ("AckCountBeforeUpdate",
                     "The number of acks needed before CWND updates.",
                     MakeTraceSourceAccessor (&TcpCubic::m_acksNeeded),
                     "ns3::TracedValueCallback::Unit32")
    .AddTraceSource ("DelayedAcks",
                     "Estimates the ratio of packets acked.",
                     MakeTraceSourceAccessor (&TcpCubic::m_delayedAck),
                     "ns3::TracedValueCallback::Unit32");

  return tid;
}

/**
 * Default constructor. Values set based on ns-2 TCP Cubic impelementation.
 */
TcpCubic::TcpCubic (void)
  : m_retxThresh(3),
    m_inFastRec (false),
    m_cubeFactor((1ull << (10+3*BICTCP_HZ)) / 410),
    m_beta(819),
    m_bicScale(41),
    m_maxIncrement(16),
    m_delayedAck(2 << ACK_RATIO_SHIFT),
    m_epochStart(0),
    m_lastMax(0),
    m_lastCwnd(0),
    m_k(0.0),
    m_dMin(0),
    m_originPoint(0),
    m_tcpCwnd(0),
    m_tcpFriendly(true),
    m_ackCnt(0),
    m_cWndCnt(0),
    m_belowOrigin(true),
    m_previousTimeStamp(0),
    m_acksNeeded(0),
    m_fastConvergence(true)
{
  NS_LOG_FUNCTION (this);
  m_betaScale = 8*(BICTCP_BETA_SCALE+m_beta)/ 3 / (BICTCP_BETA_SCALE - m_beta);
  m_cubeRttScale = (m_bicScale * 10);
}

/**
 * Copy constructor.
 */
TcpCubic::TcpCubic (const TcpCubic& sock) :
  TcpNewReno(sock),
  m_recover(sock.m_recover),
  m_retxThresh(sock.m_retxThresh),
  m_inFastRec(sock.m_inFastRec),
  m_limitedTx(sock.m_limitedTx),
  m_cubeFactor(sock.m_cubeFactor),
  m_beta(sock.m_beta),
  m_cubeRttScale(sock.m_cubeRttScale),
  m_bicScale(sock.m_bicScale),
  m_betaScale(sock.m_betaScale),
  m_maxIncrement(sock.m_maxIncrement),
  m_delayedAck(sock.m_delayedAck),
  m_epochStart(sock.m_epochStart),
  m_lastMax(sock.m_lastMax),
  m_lastCwnd(sock.m_lastCwnd),
  m_k(sock.m_k),
  m_dMin(sock.m_dMin),
  m_originPoint(sock.m_originPoint),
  m_tcpCwnd(sock.m_tcpCwnd),
  m_tcpFriendly(sock.m_tcpFriendly),
  m_ackCnt(sock.m_ackCnt),
  m_cWndCnt(sock.m_cWndCnt),
  m_belowOrigin(sock.m_belowOrigin),
  m_previousTimeStamp(sock.m_previousTimeStamp),
  m_acksNeeded(sock.m_acksNeeded)
{
  NS_LOG_FUNCTION (this);
}

/**
 * Default deconstructor.
 */
TcpCubic::~TcpCubic (void)
{
}


void
TcpCubic::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0) 
    {
      measure_delay(tcb);
    }

  // TODO:
  // ns-3 needs to add a bytes in flight parameter to the call to CongestionAvoidance
  // so that this check can be made.
  // if (!tcp_is_cwnd_limited(sk, in_flight))
  //    return;


  // The ns-2 cubic implementation has a call to check
  // for slow start, but this is handled in tcp-congestion-ops
  // so it is not needed here.
  CubicUpdate(tcb);
  

  /* In dangerous area, increase slowly.
   * In theory this is tp->snd_cwnd += 1 / tp->snd_cwnd
   */
  if (m_cWndCnt >= m_acksNeeded)
    {
      // TODO:
      // In Linux CUBIC there is a check to ensure the sending congestion
      // window is smaller than a maximum possible size before updating:
      //
      // if (tp->snd_cwnd < tp->snd_cwnd_clamp)
      //
      // ns-3 does not provide a snd_cwnd_clamp variable so this code uses
      // the vlaue 30,000 segments. This comes from checking the settings
      // used in ns-2.35
      if (tcb->m_cWnd < (30000u * tcb->m_segmentSize))
        {
          tcb->m_cWnd += tcb->m_segmentSize;
        } 
      NS_LOG_DEBUG("Increment cwnd to " << tcb->m_cWnd << " there were " << m_cWndCnt << " ACKs");
      m_cWndCnt = 0;
    }
  else
    {
      m_cWndCnt +=1;
      NS_LOG_DEBUG("Not enough segments have been ACKed to increment cwnd. ACKed " << m_cWndCnt << " need " << m_acksNeeded);
    }

}


uint32_t
TcpCubic::GetSsThresh (Ptr<const TcpSocketState> tcb,
                          uint32_t bytesInFlight)
{
  NS_LOG_DEBUG("Get new SSThresh.");
  NS_LOG_DEBUG("  Current CWND is " << (tcb->m_cWnd/tcb->m_segmentSize));

  // There are two cases where TcpSocketBase will call GetSsThresh.
  // If the TCP socket state is CA_LOSS then this is due to an
  // RTO. If so then perform a reset of cubic.
  if(tcb->m_congState == TcpSocketState::CA_LOSS)
    {
      CubicReset();
    }

  uint32_t cwndSeg = tcb->GetCwndInSegments();
  m_epochStart = 0;


  if (cwndSeg < m_lastMax && m_fastConvergence)
    {
      m_lastMax = (cwndSeg * (BICTCP_BETA_SCALE + m_beta)) / (2 * BICTCP_BETA_SCALE);
    }
  else
    {
      m_lastMax = cwndSeg;
    }
  NS_LOG_DEBUG("m_lastMax updated: " << m_lastMax);
  m_lastTime = tcb->m_rcvTimestampEchoReply;

  m_lastMax = cwndSeg;

  uint32_t temp = (cwndSeg * m_beta) / BICTCP_BETA_SCALE;
  if (temp < 2U)
    {
      temp = 2;
    }

  NS_LOG_DEBUG("New ssthresh is: " << (temp * tcb->m_segmentSize));
  return temp * tcb->m_segmentSize;
}





void
TcpCubic::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                     const Time& rtt)
{
  NS_LOG_FUNCTION (this << tcb << packetsAcked << rtt);

  if(packetsAcked > 0 && tcb->m_congState == TcpSocketState::CA_OPEN)
    {
      NS_LOG_DEBUG("  Packets Acked > " << packetsAcked << " and state CA_OPEN");
      packetsAcked -= m_delayedAck >> ACK_RATIO_SHIFT;
      m_delayedAck += packetsAcked;
      NS_LOG_DEBUG("  Delayed Ack = " << m_delayedAck);
    }
}

/**
 * Return the index of the last set bit. In the original Linux implementation
 * this method is provided in the Linux Kernel. This method is copied from the
 * the ns-2 TCP package.
 */
uint32_t
TcpCubic::fls64 (uint64_t x)
{
  uint32_t h = x >> 32;
  if (h)
    {
      return fls(h) + 32;
    }
  return fls(x);
}

/**
 * Return the index of the last set bit. The ns-2 implementation of the method
 * fls64 uses this method with normal integers. This method is copied from the
 * ns-2 TCP package.
 */
int
TcpCubic::fls (int x)
{
  int r = 32;

  if (!x)
    {
      return 0;
    }
  if (!(x & 0xffff0000u))
    {
      x <<= 16;
      r -= 16;
    }
  if (!(x & 0xff000000u))
    {
      x <<= 8;
      r -= 8;
    }
  if (!(x & 0xf0000000u))
    {
      x <<= 4;
      r -= 4;
    }
  if (!(x & 0xc0000000u))
    {
      x <<= 2;
      r -= 2;
    }
  if (!(x & 0x80000000u))
    {
      x <<= 1;
      r -= 1;
    }
  return r;
}

/**
 * The CUBIC algorithm in Linux and ns-2 does not use the normal C++ pow
 * function to take the cubed root. Instead it uses its own method. While
 * this method does not produce a perfect cubed root it is what CUBIC uses.
 */
uint32_t
TcpCubic::CubicRoot (uint64_t a)
{
  uint32_t x, b, shift;
  /*
   * cbrt(x) MSB values for x MSB values in [0..63].
   * Precomputed then refined by hand - Willy Tarreau
   *
   * For x in [0..63],
   *   v = cbrt(x << 18) - 1
   *   cbrt(x) = (v[x] + 10) >> 6
   */
  static const uint8_t v[] = {
    /* 0x00 */    0,   54,   54,   54,  118,  118,  118,  118,
    /* 0x08 */  123,  129,  134,  138,  143,  147,  151,  156,
    /* 0x10 */  157,  161,  164,  168,  170,  173,  176,  179,
    /* 0x18 */  181,  185,  187,  190,  192,  194,  197,  199,
    /* 0x20 */  200,  202,  204,  206,  209,  211,  213,  215,
    /* 0x28 */  217,  219,  221,  222,  224,  225,  227,  229,
    /* 0x30 */  231,  232,  234,  236,  237,  239,  240,  242,
    /* 0x38 */  244,  245,  246,  248,  250,  251,  252,  254,
  };
  b = fls64(a);

  NS_LOG_DEBUG("  CubicRoot: a = " << a);
  NS_LOG_DEBUG("  CubicRoot: b = " << b);

  if (b < 7)
    {
      /* a in [0..63] */
      return ((uint32_t)v[(uint32_t)a] + 35) >> 6;
    }

  b = ((b * 84) >> 8) - 1;
  shift = (a >> (b * 3));
  x = ((uint32_t)(((uint32_t)v[shift] + 10) << b)) >> 6;

  NS_LOG_DEBUG("  CubicRoot: b = " << b);
  NS_LOG_DEBUG("  CubicRoot: shift = " << shift);
  NS_LOG_DEBUG("  CubicRoot: x = " << x);

  /*
   * Newton-Raphson iteration
   *                         2
   * x    = ( 2 * x  +  a / x  ) / 3
   *  k+1          k         k
   */
  // NOTE:
  // The Linux and ns-2 implementation use the method div64_64 to divide these
  // numbers as not all systems support 64 bit numbers. I could not find a
  // similar method in C++ so for now this code assumes that 64 bit numbers are
  // supported.
  //x = (2 * x + (uint32_t)div64_64(a, (uint64_t)x * (uint64_t)(x - 1)));
  x = (2 * x + (uint32_t)(a / ((uint64_t)x * (uint64_t)(x - 1))));
  x = ((x * 341) >> 10);

  NS_LOG_DEBUG("  CubicRoot: x = " << x);
  return x;
}


uint32_t
TcpCubic::tcp_time_stamp ()
{
  uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();
  return (now & 0xFFFFFFFF);
}

void
TcpCubic::measure_delay (Ptr<TcpSocketState> tcb)
{
  /* Discard delay samples right after fast recovery */
  if( tcp_time_stamp() - m_epochStart < HZ)
    {
      return;
    }

  // Calculate delay
  uint32_t delay = (tcp_time_stamp() - tcb->m_rcvTimestampEchoReply) << 3;

  NS_LOG_DEBUG("Calculate delay = (new_timestamp - echo-from-ack) << 3\n  "
    << delay << " = ( " << tcp_time_stamp() << " - " << tcb->m_rcvTimestampEchoReply
    << ") << 3" );

  if(delay == 0)
    {
      delay = 1;
      NS_LOG_DEBUG("Delay cannot be 0, set to 1");
    }

  m_previousTimeStamp = tcb->m_rcvTimestampEchoReply;

  // Take this chance to update m_dMin
  if(m_dMin == 0 || m_dMin > delay)
    {
      m_dMin = delay;
      NS_LOG_DEBUG("Update delay to " << m_dMin);
    }
}


/**
 * Get the next size of the congestion window using the CUBIC update algorithm.
 * Depending on the current situation this could be a TCP Friendly update or a
 * standard CUBIC update for a concave or convex region.
 */
void
TcpCubic::CubicUpdate (Ptr<TcpSocketState> tcb)
{
  NS_LOG_DEBUG("Run a CUBIC update: Time " << Simulator::Now ().GetSeconds () << " seconds");

  uint32_t cwndSeg = tcb->GetCwndInSegments();
  NS_LOG_DEBUG("  cwnd = " << tcb->m_cWnd << "  segment size = " << tcb->m_segmentSize );
  NS_LOG_DEBUG("  cwnd in segments = " << (tcb->m_cWnd / tcb->m_segmentSize) );
  // The suggested amount to add to the new congestion window size.
  uint32_t windowTarget = 0;
  // The new congestion window size recommended by CUBIC.
  uint32_t cnt, min_cnt = 0;

  // Increment the number of ACKs counted
  m_ackCnt += 1;


  // Only make another check for updates if the congestion window was just updated or
  // a certain amount of time has passed.
  if ( m_lastCwnd == cwndSeg &&
       (int32_t)(tcp_time_stamp() - m_lastTime) <= HZ / 32)
    {
      NS_LOG_DEBUG("  m_lastCwnd = cwndSeg = " << cwndSeg
        << " and (tcp_time_stamp - m_lastTime) <= HZ / 32 \n ("
        << tcb->m_rcvTimestampEchoReply << " - " << m_lastTime
        << ") <= " << HZ << " / 32" );
      return;
    }
  else
    {
      NS_LOG_DEBUG("  m_lastCwnd = " << m_lastCwnd << " cwndSeg = "
        << cwndSeg << " and (tcp_time_stamp - m_lastTime) <= HZ / 32 \n "
        << tcb->m_rcvTimestampEchoReply << " - " << m_lastTime << " = "
        << (tcb->m_rcvTimestampEchoReply - m_lastTime) << "  <=  "
        << HZ << " / 32 = " << (HZ / 32) );
    }


  m_lastCwnd = cwndSeg; 
  m_lastTime = tcp_time_stamp();


  // If there has not been a packet drop yet
  if (m_epochStart == 0)
    {
      // Record the beginning of an epoch.
      m_epochStart = tcp_time_stamp();
      NS_LOG_DEBUG("  Epoch start = " << m_epochStart);

      m_ackCnt = 1;
      m_tcpCwnd = cwndSeg;

      NS_LOG_DEBUG("Is Wmax < current CWND?  Wmax = " << m_lastMax << " CWND = " << cwndSeg);
      if (m_lastMax <= cwndSeg)
        {
          // The congestion window has grown beyond W_lastMax, which was Wmax
          // before the previouse congestion window drop. So the window size
          // has now grown to the previouse Wmax and K is set to 0.
          m_k = 0;
          m_originPoint = cwndSeg;
        }
      else
        {
          // The value of K is not calculated the same way in the Linux
          // implementation than it is in the orrginal paper.
          // C = 2681735677
          m_k = CubicRoot(((uint64_t)(m_lastMax - cwndSeg) * (uint64_t)m_cubeFactor));
          m_originPoint = m_lastMax;

          NS_LOG_DEBUG("  K = cubed root of (" << m_lastMax
            << " - " << cwndSeg << ") * " << m_cubeFactor );
          NS_LOG_DEBUG("  K = " << m_k << "  Origin Point set to "
            << m_originPoint );
        }
    }


  // Next calculate 't' or the current time since epoch. When the value of
  // 't' is equal to 'K' then the CUBIC algorithm should have reached the
  // origin or Wmax point.
  uint32_t t = ((tcp_time_stamp() - m_epochStart + (m_dMin>>3))
                 << BICTCP_HZ) / HZ;

   NS_LOG_DEBUG("  t = ((tcp_time_stamp() - m_epochStart + (m_dMin>>3)) << BICTCP_HZ) / HZ \n  t = (("
     << tcp_time_stamp() << " - " << m_epochStart << " + (" << m_dMin
     << ">>3)) << " << BICTCP_HZ << ") / " << HZ << "\n  t = " << t );

  // Calculate t - K for CUBIC. In Linux and ns-2 this is done with checks to
  // make sure the result is positive
  uint64_t offs;
  if (t < m_k)
    {
      offs = m_k - t;
    }
  else
   {
     offs = t - m_k;
   }

  // In the CUBIC paper the equation
  // C * (t-K)^3
  // is used to help find the CUBIC target. The Linux implementation however
  // differs from this. Instread a value reffered to as the cube RTT scale is
  // used instread of C. This cube RTT scale seems to take some expected RTT
  // into account. However the Linux implementation also indicates that the
  // units that result from this change need to be shifted so they can be in
  // a unit called BICTCP_HZ. It is not clear exactly what this unit is but
  // many of the unit convertions inside the Linux implementation seem to be
  // for converting the TCP timestamps value to some other unit based on
  // information specific to the particular architecture of the system.
  uint32_t delta = (m_cubeRttScale * offs * offs * offs) >> (10+3*BICTCP_HZ);
  NS_LOG_DEBUG("  Calculate delta = m_cubeRttScale * offset^3 >> (10+3*BICTCP_HZ):\n  "
    << delta << " = (" <<  m_cubeRttScale << " * " << offs
    << "^3) >> (10+3*" << BICTCP_HZ << ")");

  // Next this delta value is either subtracted from or added to the origin.
  // In the Linux paper it is suggested that the cubic equation can be used
  // the same when above or below the Wmax or origin value but the Linux
  // implementation differs here.
  NS_LOG_DEBUG("Check is t < K, t = " << t << " K = " << m_k);
  if (t < m_k)
    { // Below origin
      m_belowOrigin = true;
      // Make sure that delta is less than the origin point
      if (delta > m_originPoint)
        {
          windowTarget = cwndSeg + 1;
        }
      else
        {
          windowTarget = m_originPoint - delta;
          NS_LOG_DEBUG("  Below origin, W_target: " << windowTarget);
          NS_LOG_DEBUG("  W_target = Origin_point - delta");
          NS_LOG_DEBUG("  " << windowTarget << " = " << m_originPoint << " - " << delta);
        }
    }
  else
    { // Above origin
      m_belowOrigin = false;
      windowTarget = m_originPoint + delta;
      NS_LOG_DEBUG("  Above origin, W_target: " << windowTarget);
      NS_LOG_DEBUG("  W_target = Origin_point + delta");
      NS_LOG_DEBUG("  " << windowTarget << " = " << m_originPoint << " + " << delta);
    }

  
  // Next the window target is converted into a cnt or count value. CUBIC will
  // wait until enough new ACKs have arrived that a counter meets or exceeds
  // this cnt value. This is how the CUBIC implementation simulates growing
  // cwnd by values other than 1 segment size.
  NS_LOG_DEBUG("Check windowTarget > cwndSeg: " << windowTarget
    << " > " << cwndSeg);
  if (windowTarget > cwndSeg)
    {
      cnt = cwndSeg/(windowTarget - cwndSeg);
      NS_LOG_DEBUG("  cnt = cwndSeg / (windowTarget - cwndSeg): "
        << cnt << " = " << cwndSeg << " /( " << windowTarget << " - "
        << cwndSeg << ")");
    }
  else
    {
      // Make a very small increment.
      cnt = 100 * cwndSeg;
      NS_LOG_DEBUG("  cnt = 100 * cwndSeg: " << cnt << " = 100 * " << cwndSeg);
    }

  
  if (m_dMin > 0)
    {
      NS_LOG_DEBUG("  m_dMin > 0: " << m_dMin);
      /* max increment = Smax * rtt / 0.1  */
      min_cnt = (cwndSeg * HZ * 8)/(10 * m_maxIncrement * m_dMin);
      NS_LOG_DEBUG("  min_cnt = (cwndSeg * HZ * 8)/(10 * m_maxIncrement * m_dMin)\n  "
        << min_cnt << " = (" << cwndSeg << " * " << HZ << " * 8)/(10 * "
        << m_maxIncrement << " * " << m_dMin << ")" );

      /* use concave growth when the target is above the origin */
      if (cnt < min_cnt && t >= m_k)
        {
          cnt = min_cnt;
          NS_LOG_DEBUG("  cnt = min_cnt " << cnt << " = " << min_cnt );
        }
    }


  // An extra check that the target suggested works in slow start and during
  // low utilization periods.
  if (m_lastMax == (unsigned) 0 && cnt > 20)
   {
     cnt = 20;
     NS_LOG_DEBUG("m_lastMax == 0, set cnt to 20");
   }


  // Check the TCP Friendliness version to see if the congestion window should
  // be set differently.
  if (m_tcpFriendly)
    {
      cnt = CubicTcpFriendliness(tcb, cnt);
      NS_LOG_DEBUG("TCP Friendly cnt = " << cnt);
    }

  NS_LOG_DEBUG("  cnt = (cnt << ACK_RATIO_SHIFT) / m_delayedAck: cnt = ("
    << cnt << " << " << ACK_RATIO_SHIFT << ") / " << m_delayedAck);
  cnt = (cnt << ACK_RATIO_SHIFT) / m_delayedAck;
  if ( cnt == 0 ) // cannot be zero
    {
      cnt = 1;
    }


  NS_LOG_DEBUG( "cnt = " << cnt );
  m_acksNeeded = cnt;
}


/**
 * Get the next size of the congestion window when CUBIC is in the TCP
 * TCP Friendly region.
 */
uint32_t
TcpCubic::CubicTcpFriendliness (Ptr<TcpSocketState> tcb, uint32_t cnt)
{
  NS_LOG_DEBUG("  TcpFriendliness input cnt is  " << cnt);

  uint32_t cwndSeg = tcb->GetCwndInSegments();
  uint32_t max_cnt = 0;
  uint32_t scale = m_betaScale;
  uint32_t delta = (cwndSeg * scale) >> 3;

  // update tcp cwnd
  while (m_ackCnt > delta)
    {
      m_ackCnt -= delta;
      m_tcpCwnd += 1;
    }
  
  // If BIC is slower than TCP
  NS_LOG_DEBUG("  m_tcpCwnd > cwndSeg: " << m_tcpCwnd << " > " << cwndSeg);
  if (m_tcpCwnd > cwndSeg)
    {
      delta = m_tcpCwnd - cwndSeg;
      max_cnt = cwndSeg / delta;
      if (cnt > max_cnt)
        {
          cnt = max_cnt;
        }
    }

  NS_LOG_DEBUG("  TcpFriendliness has max_cnt = " << max_cnt);
  NS_LOG_DEBUG("  TcpFriendliness cnt is " << cnt);
  return cnt;
}


/**
 * Setup CUBIC variables. Reset is called at initialization and for timeouts.
 */
void
TcpCubic::CubicReset (void)
{
  m_lastMax = 0;
  m_tcpCwnd = 0;
  m_epochStart = 0;
  m_originPoint = 0;
  m_dMin = 0;
  m_ackCnt = 0;
  m_lastTime = 0;
  m_lastCwnd = 0;
}

} // namespace ns3
