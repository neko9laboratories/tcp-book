/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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
 * Test cases for TCP Cubic. The values used in these tests are based off
 * of simulations from ns-2 version 2.35.
 *
 * Run this test class with
 * ./test.py -v -s tcp-cubic-test-suite > log.out 2>&1
 * To get specific test output message use -t to print a text
 * file of specific results:
 * /test.py -t test.txt -s tcp-cubic-test-suite > log.out 2>&1
 */

#include "ns3/tcp-cubic.h"
#include "ns3/test.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
using namespace ns3;

#include<iostream>
using namespace std;

NS_LOG_COMPONENT_DEFINE ("TcpCubicTestSuite");

/*
 * Test the CubicUpdate method comes up with the correct number of ACK packets
 * to receive before updating the congestion window. This test checks four
 * calls to CubicUpdate.
 * The first call sets the value of K and checks the ACK count found.
 * The second call tests ACK count at another point while t < K.
 * The third call tests ACK count after t >= K.
 * The fourth call test ACK count at another point when t > K.
 */
class TcpCubicIncrementTest : public TestCase
{
public:
  TcpCubicIncrementTest(
    uint32_t lastMaxCwnd,
    uint32_t cWnd1,
    uint32_t segmentSize,
    uint32_t timeOfCubicUpdate,
    uint32_t ackTimestampToEcho,
    int expectedAckCnt,
    uint32_t cWnd2,
    uint32_t time2,
    int expectedAckCnt2,
    uint32_t cWnd3,
    uint32_t time3,
    int expectedAckCnt3,
    uint32_t cWnd4,
    uint32_t time4,
    int expectedAckCnt4,
    const std::string &name);

private:
  // NS-3 Test method
  virtual void DoRun (void);
  
  // Call the TCP IncrementWindow method, which for cubic calls CubicUpdate.
  void RunCubicUpdate(void);

  /*
   * Check when CubicUpdate changes the total number of ACKs needed to update
   * the congestion window.
   */
  void CubicUpdateCheck(uint32_t, uint32_t);  

  uint32_t m_lastMaxCwnd;
  uint32_t m_cWnd1;
  uint32_t m_segmentSize;
  Ptr<TcpSocketState> m_state;
  Ptr<TcpCubic> m_cong;
  uint32_t m_timeOfCubicUpdate;
  uint32_t m_ackTimestampToEcho;
  int m_expectedAckCnt;
  uint32_t m_cWnd2;
  uint32_t m_time2;
  int m_expectedAckCnt2;
  uint32_t m_cWnd3;
  uint32_t m_time3;
  int m_expectedAckCnt3;
  uint32_t m_cWnd4;
  uint32_t m_time4;
  int m_expectedAckCnt4;
  bool m_testRun; // A flag to validate that the ACK count field does update.
  int m_testStep; // A counter to see if this is the 1st, 2nd, 3rd or 4th call.
};

TcpCubicIncrementTest::TcpCubicIncrementTest (
    uint32_t lastMaxCwnd,
    uint32_t cWnd1,
    uint32_t segmentSize,
    uint32_t timeOfCubicUpdate,
    uint32_t ackTimestampToEcho,
    int expectedAckCnt,
    uint32_t cWnd2,
    uint32_t time2,
    int expectedAckCnt2,
    uint32_t cWnd3,
    uint32_t time3,
    int expectedAckCnt3,
    uint32_t cWnd4,
    uint32_t time4,
    int expectedAckCnt4,
    const std::string &name)
  : TestCase (name),
  m_lastMaxCwnd(lastMaxCwnd),
  m_cWnd1 (cWnd1),
  m_segmentSize (segmentSize),
  m_timeOfCubicUpdate(timeOfCubicUpdate),
  m_ackTimestampToEcho(ackTimestampToEcho),
  m_expectedAckCnt(expectedAckCnt),
  m_cWnd2(cWnd2),
  m_time2(time2),
  m_expectedAckCnt2(expectedAckCnt2),
  m_cWnd3(cWnd3),
  m_time3(time3),
  m_expectedAckCnt3(expectedAckCnt3),
  m_cWnd4(cWnd4),
  m_time4(time4),
  m_expectedAckCnt4(expectedAckCnt4),
  m_testRun(false),
  m_testStep(1)
{
}

void
TcpCubicIncrementTest::CubicUpdateCheck (uint32_t oldAckCount, uint32_t newAckCount)
{
  // Get the expected ACK count to test.
  uint32_t expectedAckCnt = 0;
  if(m_testStep == 1)
    {
      m_testStep++;
      expectedAckCnt = m_expectedAckCnt;
    }
  else if (m_testStep == 2)
    {
      m_testStep++;
      expectedAckCnt = m_expectedAckCnt2;
    }
  else if (m_testStep == 3)
    {
      m_testStep++;
      expectedAckCnt = m_expectedAckCnt3;
    }
  else if (m_testStep == 4)
    {
      m_testRun = true; // Set flag to show that an update to the ACK count occured
      expectedAckCnt = m_expectedAckCnt4;
    }

  // Test the expected ACK count.
  NS_TEST_ASSERT_MSG_EQ(newAckCount, expectedAckCnt,
    "CubicUpdate picked wrong number of acks before CWND update.");
}

void
TcpCubicIncrementTest::RunCubicUpdate ()
{
  // Set the starting cwnd before calling to update it.
  uint32_t cwnd = 0;
  if(m_testStep == 1)
    {
      cwnd = m_cWnd1;
    }
  else if (m_testStep == 2)
    {
      cwnd = m_cWnd2;
    }
  else if (m_testStep == 3)
    {
      cwnd = m_cWnd3;
    }
  else if (m_testStep == 4)
    {
      cwnd = m_cWnd4;
    }

  m_state->m_cWnd = cwnd;
  m_state->m_rcvTimestampEchoReply = m_ackTimestampToEcho;

  // Call as if a TCP packet came in
  m_cong->IncreaseWindow (m_state, 1);
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
TcpCubicIncrementTest::DoRun (void)
{

  // Set the simulator clock to run long enough for the test cases used.
  Simulator::Stop (Seconds (40));


  // Schedule the four calls to RunCubicUpdate
  Simulator::Schedule (MilliSeconds (m_timeOfCubicUpdate),
                       &TcpCubicIncrementTest::RunCubicUpdate, this);

  Simulator::Schedule (MilliSeconds (m_time2),
                       &TcpCubicIncrementTest::RunCubicUpdate, this);

  Simulator::Schedule (MilliSeconds (m_time3),
                       &TcpCubicIncrementTest::RunCubicUpdate, this);

  Simulator::Schedule (MilliSeconds (m_time4),
                       &TcpCubicIncrementTest::RunCubicUpdate, this);


  // Log the test.
  LogComponentEnable("TcpCubic", LOG_LEVEL_INFO);
  LogComponentEnable("TcpCongestionOps", LOG_LEVEL_INFO);
  LogComponentEnable("TcpCubicTestSuite", LOG_LEVEL_INFO);


  // Setup the basic TCP state for the test
  m_state = CreateObject<TcpSocketState> ();
  m_state->m_cWnd = m_lastMaxCwnd;
  m_state->m_segmentSize = m_segmentSize;

  // Set Cubic as the congestion control algorithm
  m_cong = CreateObject <TcpCubic> ();


  // Trace updates to the ACK counts needed to update CWND
  m_cong->TraceConnectWithoutContext ("AckCountBeforeUpdate",
    MakeCallback (&TcpCubicIncrementTest::CubicUpdateCheck, this));


  // Simulate a dropped packet
  // Set the TCP state to look like a packet loss
  m_state->m_congState = TcpSocketState::CA_LOSS;

  // Get a new SSThresh, which will reset the Cubic parameters.
  // The second param is bytes in flight which is not used.
  m_state->m_ssThresh = m_cong->GetSsThresh(m_state, 1000);

  // Start the simulation and testing
  Simulator::Run ();

  // Cleanup
  Simulator::Destroy ();

  // Double check the test flag to make sure the ACK count needed for CUBIC
  // updates.
  NS_TEST_ASSERT_MSG_EQ(m_testRun, true,
    "There were no updates to ACK count from CUBIC update.");
}



/*
 * Unit test of the GetSsthresh method.
 */
class TcpCubicGetSsthreshTest : public TestCase
{
public:
  TcpCubicGetSsthreshTest(
    uint32_t lastMaxCwnd,
    uint32_t cWnd,
    uint32_t segmentSize,
    uint32_t expectedSsThresh,
    const std::string &name);

private:
  // NS-3 Test method
  virtual void DoRun (void);
  
    uint32_t m_lastMaxCwnd;
    uint32_t m_cWnd;
    uint32_t m_segmentSize;
    uint32_t m_expectedSsThresh;
};

TcpCubicGetSsthreshTest::TcpCubicGetSsthreshTest (
    uint32_t lastMaxCwnd,
    uint32_t cWnd,
    uint32_t segmentSize,
    uint32_t expectedSsThresh,
    const std::string &name)
  : TestCase (name),
  m_lastMaxCwnd(lastMaxCwnd),
  m_cWnd (cWnd),
  m_segmentSize (segmentSize),
  m_expectedSsThresh(expectedSsThresh)
{
}


//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
TcpCubicGetSsthreshTest::DoRun (void)
{

  // Log the test.
  LogComponentEnable("TcpCubic", LOG_LEVEL_INFO);
  LogComponentEnable("TcpCongestionOps", LOG_LEVEL_INFO);
  LogComponentEnable("TcpCubicTestSuite", LOG_LEVEL_INFO);

  // Setup the basic TCP state for the test
  Ptr<TcpSocketState> state = CreateObject<TcpSocketState> ();
  state->m_segmentSize = m_segmentSize;

  // Set Cubic as the congestion control algorithm
  Ptr<TcpCubic> cong = CreateObject <TcpCubic> ();

  // Simulate a dropped packet
  // Set the TCP state to look like a packet loss
  state->m_congState = TcpSocketState::CA_LOSS;

  // Need to set the CUBIC variable lastMax. This cannot be set directly as it
  // is a protected variable. Running GetSsThresh should set it. Run GetSsThresh
  // once to set lastMax, then call again to test the output of GetSsthresh.
  // If there is a bug in the method then this first call may not work as expexted.
  // First set cwnd to what we want last max to be. Since last max starts at 0
  // calling GetSsThresh should set ssthresh to the current cwnd.
  state->m_cWnd = m_lastMaxCwnd;
  cong->GetSsThresh(state, 1000);

  // Now that setup is complete run the test
  state->m_cWnd = m_cWnd;
  NS_TEST_ASSERT_MSG_EQ(cong->GetSsThresh(state, 1000), m_expectedSsThresh,
    "Calculated ssthresh not expected");   
}


/*
 * Unit test of the PktsAcked method.
 */
class TcpCubicPktsAckedTest : public TestCase
{
public:
  TcpCubicPktsAckedTest(
    uint32_t packetsAcked,
    uint32_t expectedDelayedAck,
    const std::string &name);

private:
  // NS-3 Test method
  virtual void DoRun (void);

  /*
   * Check when PktsAcked called the delayed ack count is adjusted properly
   */
  void DelayedAckCheck(uint32_t, uint32_t); 
  
  uint32_t m_packetsAcked;
  uint32_t m_expectedDelayedAck;
  bool m_testRun; // A flag to validate that the ACK count field does update.
};

TcpCubicPktsAckedTest::TcpCubicPktsAckedTest (
    uint32_t packetsAcked,
    uint32_t expectedDelayedAck,
    const std::string &name)
  : TestCase (name),
  m_packetsAcked(packetsAcked),
  m_expectedDelayedAck(expectedDelayedAck),
  m_testRun(false)
{
}


void
TcpCubicPktsAckedTest::DelayedAckCheck (uint32_t oldDelayAcked, uint32_t newDelayAcked)
{
  m_testRun = true;
  NS_TEST_ASSERT_MSG_EQ(newDelayAcked, m_expectedDelayedAck,
    "PktsAcked set different delayed ack than expected.");
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
TcpCubicPktsAckedTest::DoRun (void)
{

  // Log the test.
  LogComponentEnable("TcpCubic", LOG_LEVEL_INFO);
  LogComponentEnable("TcpCongestionOps", LOG_LEVEL_INFO);
  LogComponentEnable("TcpCubicTestSuite", LOG_LEVEL_INFO);

  // Setup the basic TCP state for the test
  Ptr<TcpSocketState> state = CreateObject<TcpSocketState> ();

  // Set Cubic as the congestion control algorithm
  Ptr<TcpCubic> cong = CreateObject <TcpCubic> ();

  state->m_congState = TcpSocketState::CA_OPEN;

 
  // Trace updates to the ACK counts needed to update CWND
  cong->TraceConnectWithoutContext ("DelayedAcks",
    MakeCallback (&TcpCubicPktsAckedTest::DelayedAckCheck, this));

  // RTT doesn't impact the CUBIC implementation for PktsAcked so leave this 0.
  Time test;
  cong->PktsAcked (state, m_packetsAcked, test);

  // Double check the test flag to make sure the ACK count needed for CUBIC
  // updates.
  NS_TEST_ASSERT_MSG_EQ(m_testRun, true,
    "There were no updates to delayed acks");
}


// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
static class TcpCubicTestSuite : public TestSuite
{
public:
  TcpCubicTestSuite () : TestSuite ("tcp-cubic-test-suite", UNIT)
  {

    /* Test CuibicUpdate method.
     * These tests setup the TCP environment so the CubicUpdate method can
     * be unit tested. The following input values and expected result are
     * based on test simulations of CUBIC in ns 2.35.
     * Arguments in test:
     *   cwnd (bytes) for last max CWND (before packet loss)
     *   cwnd1 (bytes) when CubicUpdate is called the first time (sets K)
     *   segment size (bytes)
     *   time (ms) when CubicUpdate is called
     *   time (ms) set on the ack packet to echo, used to figure delay
     *   Expected number of ACKs CubicUpdate says are needed to update CWND for the 1st test
     *   cwnd (bytes) when CubicUpdate is called the 2nd time.
     *   time (ms) when CubicUpdate is called the 2nd time
     *   The 2nd expected ack count to test.
     *   cwnd (bytes) when CubicUpdate is called the 3nd time, right after t >= K
     *   time (ms) when CubicUpdate is called the 3rd time
     *   The 3rd expected ack count to test.
     *   cwnd (bytes) when CubicUpdate is called the 4th time.
     *   time (ms) when CubicUpdate is called the 4th time
     *   The 4th expected ack count to test.
     *
     * The values for the congestion windows, timestamps and expected ACK
     * counts come from a test run of the TCP Cubic implementation in
     * ns-2.35.
     */
    AddTestCase (
       new TcpCubicIncrementTest (556904, 445416, 536, 2436, 2309, 41,
         472752, 3155, 55,
         556904, 10358, 51950,
         562264, 13465, 262,
         "CubicUpdate test case 1" ), TestCase::QUICK);

    AddTestCase (
       new TcpCubicIncrementTest (621760, 553152, 536, 25276, 25149, 64,
         618544, 29560, 577,
         621760, 32002, 58000,
         687152, 38874, 80,
         "CubicUpdate test case 2" ), TestCase::QUICK);


    /* Test GetSsthresh method.
     * Test the CUBIC method of setting the slow start threshold.
     * Parameters:
     *   Last Max cwnd (bytes) used to see how ssthresh is set.
     *   cwnd (bytes) when to test getting the new ssthresh.
     *   segment size (bytes)
     *   expected ssthresh (bytes) to compare the test against.
     *
     * The values used in these tests come from a test run of the TCP Cubic
     * implementation in ns-2.35.
     */
    // Test case where cwnd is greater than last max cwnd
    AddTestCase (
       new TcpCubicGetSsthreshTest (698408, 697872, 536, 557976,
         "GetSsthresh where cwnd > last max" ), TestCase::QUICK);

    // Test case where cwnd is less than last max cwnd
    AddTestCase (
       new TcpCubicGetSsthreshTest (697872, 691440, 536, 552616,
         "GetSstrhesh where cwnd < last max" ), TestCase::QUICK);



   /* Test PktsAcked method.
    * Test the CUBIC method for setting the delayed ack value based on the
    * number of packets acked. In the CUBIC code in Linux this is handled
    * in the CUBIC method bictcp_acked, which is called as the CUBIC
    * implementation of pkts_acked, and in ns-3 this relates to the method
    * PktsAcked.
    * Parameters:
    *   Number of packets acked.
    *   The expected value of delayed ack set.
    *
    * The values used in these tests come from a test run of the TCP Cubic
    * implementation in ns-2.35.
    */
    AddTestCase (
       new TcpCubicPktsAckedTest (1, 31, "PktsAcked test 1" ), TestCase::QUICK);

  }
} g_tcpCubicTestSuite;

