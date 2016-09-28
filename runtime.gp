set ylabel 'time(sec)'
set xlabel 'input to clz'
set term png enhanced font 'Verdana,10'
set output 'runtime.png'

plot 'output.txt'
