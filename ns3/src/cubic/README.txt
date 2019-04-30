Author: Brett Levasseur
email: brettl@ll.mit.edu

WPI and MIT Lincoln Laboratory


README

This code provides an implementation of the CUBIC TCP congestion
control algorithm for use with ns-3.


Installation
-------------------------------------------------------------------------------
To install this software first download a copy of the 3.27 version of
the ns-3 simulator from http://www.nsnam.org/releases/. Before
following the install process for ns-3 follow these steps:

1. Take the files from this package's src/internet/model and add them to the
   ns-3 package's src/internet/model.

2. Replace the ns-3 file src/internet/wscript with the file
   src/internet/wscript from this package.

3. Add the file src/internet/test/tcp-cubic-test-suite.cc to the ns-3 directory
   src/internet/test.

4. Continue with the standard ns-3 install instructions.


Running
-------------------------------------------------------------------------------
This CUBIC implementation runs like the other TCP congestion control
algorithms that come with ns-3. In any simulation using ns-3 simply
specify the TCP socket type to use TcpCubic. An example of using Cubic
is available in the file scratch/cubic-test.cc. To run this file place
this package's scratch/cubic-test.cc into the scratch directory of the
ns-3 installation. Then run the program with the normal WAF command
with the test class name, like ./waf --run cubic-test


Testing
-------------------------------------------------------------------------------
This implementation includes a set of tests that compare the output of this CUBIC
implementation to an example simulation run in the ns-2 (version 2.35) CUBIC
implementation. To run the test first ensure ns-3 has tests enabled by building
with the enable tests flag (./build.py --enable-examples --enable-tests) and then
use the standard ns-3 test.py script and use the package name tcp-cubic-test-suite
like the following:

./test.py -s tcp-cubic-test-suite


Future Work
-------------------------------------------------------------------------------

Additional Methods:
The CUBIC implementation in Linux includes methods that need to be called during
normal TCP stages. These include the methods:

Basic TCP method     CUBIC equivalent
init		      bictcp_init
ssthresh	      bictcp_recalc_ssthresh
cong_avoid	      bictcp_cong_avoid
set_state	      bictcp_state
undo_cwnd	      bictcp_undo_cwnd
pkts_acked            bictcp_acked

This ns-3 implementation handles sshthresh recalculation and congestion avoidance.
However, the current version of ns-3 (3.27) does not appear to allow congestion
control algorithms to implement their own code for init, set_state, undo_cwnd and
pkts_acked. While current testing indicates that not implementing these methods in
ns-3 does not make a difference there are likely cases where their absence will
cause simulation inaccuracies. The ns-3 base TCP code needs to allow for these
methods to be over-ridden by the congestion control classes.


TCP Socket Structure:
In the Linux TCP CUBIC code the TCP Socket structure tcp_sock includes fields
that are used by CUBIC that are not available in the current ns-3 (3.27) classes
TcpSocket or TcpSocketState. One field used in CUBIC is:

snd_cwnd_clamp

This field is an unsigned 16 bit number that represents a size that the send
congestion window cannot grow above. This field should be added to the regular
ns-3 code base so that it can be set and used in congestion control algorithms.
Currently this CUBIC implementation just ensures that the congestion window cannot
be set larger than its maximum value for a 32 bit unsigned integer. We use 32 bit
instead of 16 bit because ns-3 is working in congestion windows in units of bytes
while the ns-2 and Linux implementation work in number of segments.

Another question is what value should be set for snd_cwnd_clamp in CUBIC. The
CUBIC code does not set a specific value to snd_cwnd_clamp. TCP Hyla sets the
clamp at 65,535, while TCP High Speed and TCP Yeah limits it the minimum of the
current value or 0xffffffff/128 (around 33,554,431). Tests run in ns-2 version 2.35
show that the clamp is set by default to 30,000 segments so that is the value
used in this implementation.


Bytes In Flight:
The CUBIC method bitctp_cong_avoid (CongestionAvoidance in ns-3) takes in the
parameter in_flight to track how many bytes are currently in flight. This is
used to trigger the method to return when TCP is congestion window limited. The
ns-3 implementation needs to add the number of bytes in flight to the
CongestionAvoidance method to allow for this check.


ACK Timeout:
CUBIC should perform a reset when the ACK timeout expires but there does not
appear to be a way for this timeout to signal this in ns-3.26 so this is not
implemented. This is currently handled in the method DelAckTimeout in
TcpSocketBase.



Change Log
-------------------------------------------------------------------------------
11/15/2017
  As of ns-3.27 the new field m_rcvTimestampEchoReply implements the TCP
  timestamp echo. As a result the changed this CUBIC implementation made
  to TCP Socket Base are no longer needed. This package therefore only
  contains new files and modified wscript files for building. The relevant
  CUBIC code was modified to use m_rcvTimestampEchoReply.


1/8/2017
  Re-factored the code base to work in the new 3.26 version of ns-3.

  Added tests based on results from ns-2 (version 2.35) implementation of CUBIC
  for validation.

  Added the TCP timestamp to TcpSocketState (required for CUBIC).

  Bug fixes.


3/21/2016
  Fixed use of TCP fast recovery in NewAck to the NewReno implementation.

  Removed some global variables and some methods that are provided by TCP socket
  base, correcting issue with ssthresh setting.


11/22/2015
  Update source to support ns-3 version 3.24. Add support for TCP timestamp
  feature.

  Corrected bug with units of m_dMin.


3/24/2014
  Initial version presented at the ns-3 workshop.


