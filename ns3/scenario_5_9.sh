#!/bin/bash

sc=9
rm data/05_xx-sc${sc}-*

ALGORITHMS=(TcpBic TcpCubic)

a_bw=1Gbps
a_dl=10ms
time=100

for item in ${ALGORITHMS[@]}; do
for bw in 10Mbps; do
for dl in 10ms; do
  echo "----- Simulating $item $bw $dl -----"
  ./waf --run "chapter5-diffTcp --transport_prot=TcpNewReno --transport_prot2=$item --prefix_name='data/05_xx-sc${sc}-$item-${bw}-${dl}' --tracing=True --duration=$time --bandwidth=$bw --delay=$dl --access_bandwidth=$a_bw --access_delay=$a_dl"
  
  # gnuplot
  # throughput-comparison
	gnuplot <<- EOS
	set terminal pngcairo enhanced font "TimesNewRoman" fontscale 2.5 size 1280,960
	set output 'data/05_xx-sc${sc}-$item-${bw}-${dl}-throughput-comp.png'
	set xlabel "Time [s]"
	set ylabel "Throughput [Mbps]"
	set xrange [0:$time]
	plot "data/05_xx-sc${sc}-$item-${bw}-${dl}-flw0-throughput.data" using 1:2 title "NewReno" with lines lc rgb "black" lw 2 dt (10,0), "data/05_xx-sc${sc}-$item-${bw}-${dl}-flw1-throughput.data" using 1:2 title "${item}" with lines lc rgb "grey" lw 2 dt (10,0)
	EOS
  
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-*throughput.data /media/sf_neko9_tcpbook/ns3/data/chapter5/sc${sc}/.
  cp data/05_xx-sc${sc}-$item-${bw}-${dl}-*.png /media/sf_neko9_tcpbook/ns3/data/chapter5/sc${sc}/.
done
done
done
