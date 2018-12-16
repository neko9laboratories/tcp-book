#!/bin/bash

sc=5
rm data/05_xx-sc${sc}-*

ALGORITHMS=(TcpBic)

a_bw=1Gbps
a_dl=10ms
time=100

for item in ${ALGORITHMS[@]}; do
for bw in 10Mbps 50Mbps; do
for dl in 10ms 100ms; do
  echo "----- Simulating $item $bw $dl -----"
  ./waf --run "chapter5-base --transport_prot=$item --prefix_name='data/05_xx-sc${sc}-$item-${bw}-${dl}' --tracing=True --duration=$time --bandwidth=$bw --delay=$dl --access_bandwidth=$a_bw --access_delay=$a_dl"
  
  # gnuplot
  case "$bw" in
  "1Mbps" )   rng=2.0;;
  "10Mbps" )  rng=20 ;;
  "50Mbps" )  rng=100;;
  "100Mbps" ) rng=200;;
  esac
  tcs=`expr $rng/10`
  # cwnd
  for flw in 0; do
	gnuplot <<- EOS
	set terminal pngcairo enhanced font "TimesNewRoman" fontscale 1.25
	set output 'data/05_xx-sc${sc}-$item-${bw}-${dl}-flw${flw}-cwnd.png'
	set xlabel "Time [s]"
	set ylabel "Window size [byte]"
	set y2label "Throughput [Mbps]"
	set xrange [0:$time]
	set ytics nomirror
	set y2range [0:$rng]
	set y2tics $tcs
	f(x)=65535
	plot "data/05_xx-sc${sc}-$item-${bw}-${dl}-flw${flw}-cwnd.data" using 1:2 axis x1y1 title "Cwnd" with lines lc rgb "grey" lw 2 dt (10,0), f(x) axis x1y1 title "Rwnd" with lines lc rgb "dark-grey" lw 2 dt (5,5), "data/05_xx-sc${sc}-$item-${bw}-${dl}-flw${flw}-throughput.data" using 1:2 axis x1y2 title "Throughput" with lines lc rgb "black" lw 2 dt (10,0)
	EOS
  
  # RTT
	gnuplot <<- EOS
	set terminal pngcairo enhanced font "TimesNewRoman" fontscale 1.25
	set output 'data/05_xx-sc${sc}-$item-${bw}-${dl}-flw${flw}-rtt.png'
	set xlabel "Time [s]"
	set ylabel "RTT [s]"
	set xrange [0:$time]
	plot "data/05_xx-sc${sc}-$item-${bw}-${dl}-flw${flw}-rtt.data" using 1:2 notitle with lines lc rgb "grey" lw 2 dt (10,0)
	EOS
  
  # cong-state
	gnuplot <<- EOS
	set terminal pngcairo enhanced font "TimesNewRoman" fontscale 1.25
	set output 'data/05_xx-sc${sc}-$item-${bw}-${dl}-flw${flw}-cong-state.png'
	set xlabel "Time [s]"
	set ylabel "State"
	set xrange [0:$time]
	set yrange [0:4.5]
	set ytics 1
	plot "data/05_xx-sc${sc}-$item-${bw}-${dl}-flw${flw}-cong-state.data" using 1:2 notitle with steps lc rgb "grey" lw 2 dt (10,0)
	EOS
  done
  
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-*cwnd.data /media/sf_neko9_tcpbook/ns3/data/sc${sc}/.
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-*rtt.data /media/sf_neko9_tcpbook/ns3/data/sc${sc}/.
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-*cong-state.data /media/sf_neko9_tcpbook/ns3/data/sc${sc}/.
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-*throughput.data /media/sf_neko9_tcpbook/ns3/data/sc${sc}/.
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-queue-*.data /media/sf_neko9_tcpbook/ns3/data/sc${sc}/.
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-*.png /media/sf_neko9_tcpbook/ns3/data/sc${sc}/.
done
done
done
