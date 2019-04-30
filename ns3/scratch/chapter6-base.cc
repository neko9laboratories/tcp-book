/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 * Copyright (c) 2019 Yu Nakayama, Ryoma Yasunaga
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
 *
 * Authors: Justin P. Rohrer, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>, Siddharth Gangadhar <siddharth@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 *
 * “TCP Westwood(+) Protocol Implementation in ns-3”
 * Siddharth Gangadhar, Trúc Anh Ngọc Nguyễn , Greeshma Umapathi, and James P.G. Sterbenz,
 * ICST SIMUTools Workshop on ns-3 (WNS3), Cannes, France, March 2013
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpVariantsComparison");

bool firstCwnd = true;
bool firstSshThr = true;
bool firstRtt = true;
bool firstRto = true;
Ptr<OutputStreamWrapper> cWndStream;
Ptr<OutputStreamWrapper> ssThreshStream;
Ptr<OutputStreamWrapper> rttStream;
Ptr<OutputStreamWrapper> rtoStream;
Ptr<OutputStreamWrapper> nextTxStream;
Ptr<OutputStreamWrapper> nextRxStream;
Ptr<OutputStreamWrapper> inFlightStream;
Ptr<OutputStreamWrapper> ackStream;
Ptr<OutputStreamWrapper> congStateStream;
uint32_t cWndValue;
uint32_t ssThreshValue;
double TH_INTERVAL = 5.0;

static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue = newval;

  if (!firstSshThr)
    {
      *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << ssThreshValue << std::endl;
    }
}

static void
SsThreshTracer (uint32_t oldval, uint32_t newval)
{
  if (firstSshThr)
    {
      *ssThreshStream->GetStream () << "0.0 " << oldval << std::endl;
      firstSshThr = false;
    }
  *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  ssThreshValue = newval;

  if (!firstCwnd)
    {
      *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << cWndValue << std::endl;
    }
}

static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (Time oldval, Time newval)
{
  if (firstRto)
    {
      *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRto = false;
    }
  *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
NextTxTracer (SequenceNumber32 old, SequenceNumber32 nextTx)
{
  *nextTxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
}

static void
InFlightTracer (uint32_t old, uint32_t inFlight)
{
  *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
}

static void
NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
{
  *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
}

static void
AckTracer (SequenceNumber32 old, SequenceNumber32 newAck)
{
  *ackStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newAck << std::endl;
}

static void
CongStateTracer (TcpSocketState::TcpCongState_t old, TcpSocketState::TcpCongState_t newState)
{
  *congStateStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newState << std::endl;
}

static void
TraceCwnd (uint32_t nodeId, std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&CwndTracer));
  //Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
}

static void
TraceSsThresh (uint32_t nodeId, std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  ssThreshStream = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&SsThreshTracer));
}

static void
TraceRtt (uint32_t nodeId, std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/RTT";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&RttTracer));
}

static void
TraceRto (uint32_t nodeId, std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/RTO";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&RtoTracer));
}

static void
TraceNextTx (uint32_t nodeId, std::string &next_tx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextTxStream = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&NextTxTracer));
}

static void
TraceInFlight (uint32_t nodeId, std::string &in_flight_file_name)
{
  AsciiTraceHelper ascii;
  inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&InFlightTracer));
}

static void
TraceNextRx (uint32_t nodeId, std::string &next_rx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextRxStream = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&NextRxTracer));
}

static void
TraceAck (uint32_t nodeId, std::string &ack_file_name)
{
  AsciiTraceHelper ascii;
  ackStream = ascii.CreateFileStream (ack_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/HighestRxAck";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&AckTracer));
}

static void
TraceCongState (uint32_t nodeId, std::string &cong_state_file_name)
{
  AsciiTraceHelper ascii;
  congStateStream = ascii.CreateFileStream (cong_state_file_name.c_str ());
  std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/CongState";
  Config::ConnectWithoutContext (nodelist, MakeCallback (&CongStateTracer));
}

static std::string
GetTcpAlgorithm (std::string transport_prot)
{
	std::string alg;
	if (transport_prot.compare ("TcpNewReno") == 0) {
		alg = "ns3::TcpNewReno";
	} else if (transport_prot.compare ("TcpHybla") == 0) {
		alg = "ns3::TcpHybla";
	} else if (transport_prot.compare ("TcpHighSpeed") == 0) {
		alg = "ns3::TcpHighSpeed";
	} else if (transport_prot.compare ("TcpVegas") == 0) {
		alg = "ns3::TcpVegas";
	} else if (transport_prot.compare ("TcpScalable") == 0) {
		alg = "ns3::TcpScalable";
	} else if (transport_prot.compare ("TcpHtcp") == 0) {
		alg = "ns3::TcpHtcp";
	} else if (transport_prot.compare ("TcpVeno") == 0) {
		alg = "ns3::TcpVeno";
	} else if (transport_prot.compare ("TcpBic") == 0) {
		alg = "ns3::TcpBic";
	} else if (transport_prot.compare ("TcpCubic") == 0) {
		alg = "ns3::TcpCubic";
	} else if (transport_prot.compare ("TcpBbr") == 0) {
		alg = "ns3::TcpBbr";
	} else {
		NS_LOG_DEBUG ("Invalid TCP version");
		exit (1);
	}
	return alg;
}

static void
SetTcpAlgorithm2Node (uint32_t nodeId, std::string transport_prot)
{
	std::string nodelist = "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketType";
	TypeId tid = TypeId::LookupByName(GetTcpAlgorithm(transport_prot));
	Config::Set(nodelist, TypeIdValue(tid));
}

static void // trace throughput in Mbps
TraceThroughput (Ptr<Application> app, Ptr<OutputStreamWrapper> stream, uint32_t oldTotalBytes)
{
	Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
	uint32_t newTotalBytes = pktSink->GetTotalRx ();
	*stream->GetStream() << Simulator::Now ().GetSeconds () << "\t" << (newTotalBytes - oldTotalBytes)*8.0/TH_INTERVAL/1000/1000 << std::endl;
	Simulator::Schedule (Seconds (TH_INTERVAL), &TraceThroughput, app, stream, newTotalBytes);
}

static void
StartAppTrace(ApplicationContainer sinkapp, std::string th_file_name)
{
	AsciiTraceHelper ascii;
	Ptr<OutputStreamWrapper> st1 = ascii.CreateFileStream(th_file_name);
	Simulator::Schedule (Seconds (TH_INTERVAL), &TraceThroughput, sinkapp.Get(0), st1, 0);
}

static void
TraceQueue (Ptr< Queue< Packet > > queue, Ptr<OutputStreamWrapper> stream, std::string type)
{
	uint32_t sizeB = queue->GetNBytes ();
	uint32_t sizeP = queue->GetNPackets ();
	uint32_t  recB = queue->GetTotalReceivedBytes ();
	uint32_t  recP = queue->GetTotalReceivedPackets ();
	uint32_t dropB = queue->GetTotalDroppedBytes ();
	uint32_t dropP = queue->GetTotalDroppedPackets ();

	if(type.compare("bytes") == 0) {
		*stream->GetStream() << Simulator::Now ().GetSeconds () << "\t" << sizeB << "\t" << recB << "\t" << dropB << std::endl;
	} else {
		*stream->GetStream() << Simulator::Now ().GetSeconds () << "\t" << sizeP << "\t" << recP << "\t" << dropP << std::endl;
	}
	Simulator::Schedule (Seconds (TH_INTERVAL), &TraceQueue, queue, stream, type);
}

static void
StartQueueTrace (Ptr<NetDevice> dev, std::string type, std::string q_file_name)
{
	Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (dev);
	Ptr< Queue< Packet > > queue = nd->GetQueue ();

	AsciiTraceHelper ascii;
	Ptr<OutputStreamWrapper> st1 = ascii.CreateFileStream(q_file_name);
	*st1->GetStream() << "Time\t" << "size\t" << "received\t" << "dropped" << "\n";
	Simulator::Schedule (Seconds (TH_INTERVAL), &TraceQueue, queue, st1, type);
}

static PointToPointHelper
GetP2PLink (std::string bandwidth, std::string delay, uint32_t q_size)
{
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
	p2p.SetChannelAttribute ("Delay", StringValue (delay));
	p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (q_size));
	return p2p;
}

int main (int argc, char *argv[])
{
  std::string transport_prot = "TcpCubic";
  double error_p = 0.0;
  std::string bandwidth = "1Mbps";
  std::string delay = "1ms";
  std::string access_bandwidth = "100Mbps";
  std::string access_delay = "10ms";
  bool tracing = false;
  std::string prefix_file_name = "TcpDelayBase";
  double data_mbytes = 0;
  uint32_t mtu_bytes = 1500;
  uint16_t num_flows = 1;
  float duration = 100;
  uint32_t run = 0;
  uint32_t q_size = 10;
  bool flow_monitor = false;
  bool pcap = false;


  CommandLine cmd;
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpCubic, TcpBbr, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus ", transport_prot);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("bandwidth", "Bottleneck bandwidth", bandwidth);
  cmd.AddValue ("delay", "Bottleneck delay", delay);
  cmd.AddValue ("access_bandwidth", "Access link bandwidth", access_bandwidth);
  cmd.AddValue ("access_delay", "Access link delay", access_delay);
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("prefix_name", "Prefix of output trace file", prefix_file_name);
  cmd.AddValue ("data", "Number of Megabytes of data to transmit", data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("num_flows", "Number of flows", num_flows);
  cmd.AddValue ("duration", "Time to allow flows to run in seconds", duration);
  cmd.AddValue ("run", "Run index (for setting repeatable seeds)", run);
  cmd.AddValue ("q_size", "Queue size", q_size);
  cmd.AddValue ("flow_monitor", "Enable flow monitor", flow_monitor);
  cmd.AddValue ("pcap_tracing", "Enable or disable PCAP tracing", pcap);
  cmd.Parse (argc, argv);

  SeedManager::SetSeed (1);
  SeedManager::SetRun (run);

  // User may find it convenient to enable logging
  //LogComponentEnable("TcpVariantsComparison", LOG_LEVEL_ALL);
  //LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
  //LogComponentEnable("PfifoFastQueueDisc", LOG_LEVEL_ALL);

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

  // Set the simulation start and stop time
  float start_time = 0.1;
  float stop_time = start_time + duration;

  // 4 MB of TCP buffer
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));

  //Config::SetDefault ("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (65535));
  //Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));

  // Create gateways, sources, and sinks
  NodeContainer gateways;
  gateways.Create (5);
  NodeContainer sources;
  sources.Create (num_flows);
  NodeContainer sinks;
  sinks.Create (num_flows);

  // Configure the error model
  // Here we use RateErrorModel with packet error rate
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);

  PointToPointHelper LocalLink = GetP2PLink ("100Mbps", access_delay, q_size);
  PointToPointHelper GwLink1 = GetP2PLink ("80Mbps", delay, q_size);
  PointToPointHelper GwLink2 = GetP2PLink ("60Mbps", delay, q_size);
  PointToPointHelper GwLink3 = GetP2PLink ("40Mbps", delay, q_size);
  PointToPointHelper GwLink4 = GetP2PLink ("20Mbps", delay, q_size);
  PointToPointHelper UnReLink = GetP2PLink ("10Mbps", delay, q_size);

  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer sink_interfaces;

  for (int i = 0; i < num_flows; i++)
    {
      int q = 0;
      NetDeviceContainer devices;
      devices = LocalLink.Install (sources.Get (i), gateways.Get (0));
      address.NewNetwork ();
      Ipv4InterfaceContainer interfaces = address.Assign (devices);

      if(i == 0) {
        devices = GwLink1.Install (gateways.Get (0), gateways.Get (1));
        address.NewNetwork ();
        interfaces = address.Assign (devices);
        StartQueueTrace(devices.Get(0), "packets", prefix_file_name + "-queue-" + std::to_string(q++) + ".data");

        devices = GwLink2.Install (gateways.Get (1), gateways.Get (2));
        address.NewNetwork ();
        interfaces = address.Assign (devices);
        StartQueueTrace(devices.Get(0), "packets", prefix_file_name + "-queue-" + std::to_string(q++) + ".data");

        devices = GwLink3.Install (gateways.Get (2), gateways.Get (3));
        address.NewNetwork ();
        interfaces = address.Assign (devices);
        StartQueueTrace(devices.Get(0), "packets", prefix_file_name + "-queue-" + std::to_string(q++) + ".data");

        devices = GwLink4.Install (gateways.Get (3), gateways.Get (4));
        address.NewNetwork ();
        interfaces = address.Assign (devices);
        StartQueueTrace(devices.Get(0), "packets", prefix_file_name + "-queue-" + std::to_string(q++) + ".data");
      }

      devices = UnReLink.Install (gateways.Get (4), sinks.Get (i));
      address.NewNetwork ();
      interfaces = address.Assign (devices);
      StartQueueTrace(devices.Get(0), "packets", prefix_file_name + "-queue-" + std::to_string(q++) + ".data");

      sink_interfaces.Add (interfaces.Get (1));

    }

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  for (uint16_t i = 0; i < sources.GetN (); i++)
    {
      uint16_t port = 50000 + i;
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

      AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (i, 0), port));

      SetTcpAlgorithm2Node (sources.Get (i)->GetId(), transport_prot);

      Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
      BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
      ftp.SetAttribute ("Remote", remoteAddress);
      ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
      ftp.SetAttribute ("MaxBytes", UintegerValue (int(data_mbytes * 1000000)));

      ApplicationContainer sourceApp = ftp.Install (sources.Get (i));
      sourceApp.Start (Seconds (start_time * i));
      sourceApp.Stop (Seconds (stop_time));

      sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = sinkHelper.Install (sinks);
      sinkApp.Start (Seconds (start_time * i));
      sinkApp.Stop (Seconds (stop_time));

      StartAppTrace(sinkApp, prefix_file_name + "-flw" + std::to_string(i) + "-throughput.data");
    }

  // Set up tracing if enabled
  if (tracing)
    {
      /*
      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
                                            std::ios::out);
      stack.EnableAsciiIpv4All (ascii_wrap);
      */

      for (int i = 0; i < 1; i++) {
        Simulator::Schedule (Seconds (0.00001), &TraceCwnd, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-cwnd.data");
        Simulator::Schedule (Seconds (0.00001), &TraceSsThresh, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-ssth.data");
        Simulator::Schedule (Seconds (0.00001), &TraceRtt, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-rtt.data");
        Simulator::Schedule (Seconds (0.00001), &TraceRto, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-rto.data");
        Simulator::Schedule (Seconds (0.00001), &TraceNextTx, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-next-tx.data");
        Simulator::Schedule (Seconds (0.00001), &TraceInFlight, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-inflight.data");
        Simulator::Schedule (Seconds (0.1), &TraceNextRx, sinks.Get (i)->GetId(),  prefix_file_name + "-flw" + std::to_string(i) + "-next-rx.data");
        Simulator::Schedule (Seconds (0.00001), &TraceAck, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-ack.data");
        Simulator::Schedule (Seconds (0.00001), &TraceCongState, sources.Get (i)->GetId(), prefix_file_name + "-flw" + std::to_string(i) + "-cong-state.data");
      }
    }

  if (pcap)
    {
      UnReLink.EnablePcapAll (prefix_file_name, true);
      LocalLink.EnablePcapAll (prefix_file_name, true);
    }

  // Flow monitor
  FlowMonitorHelper flowHelper;
  if (flow_monitor)
    {
      flowHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();

  if (flow_monitor)
    {
      flowHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", true, true);
    }

  Simulator::Destroy ();
  return 0;
}
