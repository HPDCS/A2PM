#set terminal postscript eps enhanced color font 'Helvetica,14'
#set output sprintf("gnuplot/%s.eps",the_title)

set terminal pngcairo size 1024,768 enhanced font 'Verdana,14'
set output sprintf("gnuplot/lasso/%s.png",the_title)

unset key

set yrange [0:*]
set xrange [0:*]


set title the_title
set datafile separator ","

set xlabel "RTTC/RTTH (seconds)"
set ylabel "Predicted RTTC (seconds)"

rnd(x) = floor(x/10)*10

#plot	sprintf("csv/%s.csv",the_title)	using 1:2 smooth unique,\
#		x
#plot	sprintf("csv/%s.csv",the_title)	using (bin($1,binwidth)):($2)
plot	sprintf("csv/%s.csv",the_title)	using (rnd($1)):($2) smooth unique,\
		x
