#set terminal postscript eps enhanced color font 'Helvetica,10'
#set output sprintf("gnuplot/parameters/%s.eps",the_title)

set terminal pngcairo size 1024,768 font 'Verdana,10'
set output sprintf("gnuplot/parameters/%s.png",the_title)

set key outside;
set key right top;

set xrange [0:*]
set yrange [0:*]


set xlabel 'Remaining Time to Threshold Hit (seconds)'


set title the_title
set datafile separator ","

rnd(x) = floor(x/30)*30

plot	sprintf("csv/%s.csv",the_title)  using (rnd($1)):3 smooth unique t col,\
	''   using (rnd($1)):4 smooth unique t col,\
	''   using (rnd($1)):5 smooth unique t col,\
	''   using (rnd($1)):6 smooth unique t col,\
	''   using (rnd($1)):7 smooth unique t col,\
	''   using (rnd($1)):8 smooth unique t col,\
	''   using (rnd($1)):9 smooth unique t col,\
	''   using (rnd($1)):10 smooth unique t col,\
	''   using (rnd($1)):11 smooth unique t col,\
	''   using (rnd($1)):12 smooth unique t col,\
	''   using (rnd($1)):13 smooth unique t col,\
	''   using (rnd($1)):14 smooth unique t col,\
	''   using (rnd($1)):15 smooth unique t col,\
	''   using (rnd($1)):16 smooth unique t col,\
	''   using (rnd($1)):17 smooth unique t col,\
	''   using (rnd($1)):18 smooth unique t col,\
	''   using (rnd($1)):19 smooth unique t col,\
	''   using (rnd($1)):20 smooth unique t col,\
	''   using (rnd($1)):21 smooth unique t col,\
	''   using (rnd($1)):22 smooth unique t col,\
	''   using (rnd($1)):23 smooth unique t col,\
	''   using (rnd($1)):24 smooth unique t col,\
	''   using (rnd($1)):25 smooth unique t col,\
	''   using (rnd($1)):26 smooth unique t col,\
	''   using (rnd($1)):27 smooth unique t col,\
	''   using (rnd($1)):28 smooth unique t col,\
	''   using (rnd($1)):29 smooth unique t col,\
	''   using (rnd($1)):30 smooth unique t col,\
	''   using (rnd($1)):31 smooth unique t col,\
	''   using (rnd($1)):32 smooth unique t col,\
	''   using (rnd($1)):33 smooth unique t col,\
	''   using (rnd($1)):34 smooth unique t col,\
	''   using (rnd($1)):35 smooth unique t col,\
	''   using (rnd($1)):36 smooth unique t col,\
	''   using (rnd($1)):37 smooth unique t col,\
	''   using (rnd($1)):38 smooth unique t col,\
	''   using (rnd($1)):39 smooth unique t col;
	
