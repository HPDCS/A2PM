set terminal pngcairo size 1024,768 enhanced font 'Verdana,10'
set output "gnuplot/lasso/SelectedParameters.png"


#set terminal postscript eps enhanced color font 'Helvetica,16'
#set output 'gnuplot/lasso/SelectedParameters.eps'

unset key

set logscale x

set xrange [1:*]
set yrange [0:30]

set format x "10^%T";

set xlabel '{/Symbol l} Value'
set ylabel 'Selected Parameters'

set title "Number of Parameters Selected by Lasso"

plot 'LassoParameters.dat' using 1:2 w lp
