#set terminal postscript eps enhanced color font 'Helvetica,16'
#set output 'gnuplot/parameters/all.eps'

set terminal pngcairo dashed size 1024,768 font 'Verdana,10' linewidth 2
set output "gnuplot/parameters/all.png"

set key outside;
set key right top;

set xrange [0:*]
set yrange [0:*]
set y2range [0:100]

set ytics nomirror
set y2tics nomirror

set ylabel 'Memory (MiB) or Number of Threads'
set y2label 'CPU (%) or Seconds'
set xlabel 'RTTH/RRTC (seconds)'

set title "All Parameters"
set datafile separator ","

rnd(x) = floor(x/30)*30

plot	'aggregated.csv' using (rnd($1)):2 smooth unique lt 2 lc rgb 'navy' t col axes x1y2,\
	''   using (rnd($1)):25 smooth unique lt 2 lc rgb 'gold' t col axes x1y2,\
	''   using (rnd($1)):26 smooth unique lt 2 lc rgb 'red' t col axes x1y2,\
	''   using (rnd($1)):27 smooth unique lt 2 lc rgb 'green' t col axes x1y2,\
	''   using (rnd($1)):28 smooth unique lt 2 lc rgb 'blue' t col axes x1y2,\
	''   using (rnd($1)):29 smooth unique lt 2 lc rgb 'magenta' t col axes x1y2,\
	''   using (rnd($1)):30 smooth unique lt 2 lc rgb 'cyan' t col axes x1y2,\
	''   using (rnd($1)):17 smooth unique lt 3 lc rgb 'gold' t col axes x1y1,\
	''   using (rnd($1)):($18/1024) smooth unique lt 1 lc rgb 'red' t col axes x1y1,\
	''   using (rnd($1)):($19/1024) smooth unique lt 1 lc rgb 'green' t col axes x1y1,\
	''   using (rnd($1)):($20/1024) smooth unique lt 1 lc rgb 'blue' t col axes x1y1,\
	''   using (rnd($1)):($21/1024) smooth unique lt 1 lc rgb 'magenta' t col axes x1y1,\
	''   using (rnd($1)):($22/1024) smooth unique lt 1 lc rgb 'cyan' t col axes x1y1,\
	''   using (rnd($1)):($23/1024) smooth unique lt 1 lc rgb 'navy' t col axes x1y1,\
	''   using (rnd($1)):($24/1024) smooth unique lt 1 lc rgb 'gold' t col axes x1y1;
	
