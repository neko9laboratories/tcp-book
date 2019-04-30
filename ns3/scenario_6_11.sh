#!/bin/bash

sc=11
rm data/06_xx-sc${sc}-*

ALGORITHMS=(TcpCubic)

time=100
nf=8

for item in ${ALGORITHMS[@]}; do
for q in 100 1000 10000; do
  echo "----- Simulating $item ${q} -----"
  ./waf --run "chapter6-base --transport_prot=$item --prefix_name='data/06_xx-sc${sc}-$item-${q}' --tracing=True --duration=$time --num_flows=${nf} --q_size=${q}"

  # gnuplot
  # cwnd
  #for flw in `seq 0 $nf`; do
  for flw in 0; do
	gnuplot <<- EOS
	set terminal pngcairo enhanced font "TimesNewRoman" fontscale 2.5 size 1280,960
	set output 'data/06_xx-sc${sc}-$item-${q}-flw${flw}-cwnd.png'
	set xlabel "Time [s]"
	set ylabel "Window size [byte]"
	set y2label "Throughput [Mbps]"
	set xrange [0:$time]
	set ytics nomirror
	set y2range [0:10]
	set y2tics 1
	f(x)=65535
	plot "data/06_xx-sc${sc}-$item-${q}-flw${flw}-cwnd.data" using 1:2 axis x1y1 title "Cwnd" with lines lc rgb "grey" lw 2 dt (10,0), f(x) axis x1y1 title "Rwnd" with lines lc rgb "dark-grey" lw 2 dt (5,5), "data/06_xx-sc${sc}-$item-${q}-flw${flw}-throughput.data" using 1:2 axis x1y2 title "Throughput" with lines lc rgb "black" lw 2 dt (10,0)
	EOS

  # RTT
	gnuplot <<- EOS
	set terminal pngcairo enhanced font "TimesNewRoman" fontscale 2.5 size 1280,960
	set output 'data/06_xx-sc${sc}-$item-${q}-flw${flw}-rtt.png'
	set xlabel "Time [s]"
	set ylabel "RTT [s]"
	set xrange [0:$time]
	plot "data/06_xx-sc${sc}-$item-${q}-flw${flw}-rtt.data" using 1:2 notitle with lines lc rgb "grey" lw 2 dt (10,0)
	EOS

  # cong-state
	gnuplot <<- EOS
	set terminal pngcairo enhanced font "TimesNewRoman" fontscale 2.5 size 1280,960
	set output 'data/06_xx-sc${sc}-$item-${q}-flw${flw}-cong-state.png'
	set xlabel "Time [s]"
	set ylabel "State"
	set xrange [0:$time]
	set yrange [0:4.5]
	set ytics 1
	plot "data/06_xx-sc${sc}-$item-${q}-flw${flw}-cong-state.data" using 1:2 notitle with steps lc rgb "grey" lw 2 dt (10,0)
	EOS
  done

  mv data/06_xx-sc${sc}-$item-${q}-*cwnd.data data/chapter6/sc${sc}/.
  mv data/06_xx-sc${sc}-$item-${q}-*rtt.data data/chapter6/sc${sc}/.
  mv data/06_xx-sc${sc}-$item-${q}-*cong-state.data data/chapter6/sc${sc}/.
  mv data/06_xx-sc${sc}-$item-${q}-*throughput.data data/chapter6/sc${sc}/.
  mv data/06_xx-sc${sc}-$item-${q}-queue-*.data data/chapter6/sc${sc}/.
  mv data/06_xx-sc${sc}-$item-${q}-*.png data/chapter6/sc${sc}/.
done
done
