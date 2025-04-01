# Column 1: wavelength
# Column 2: Slab width
# Column 3: mode order
# Column 4: Position
# Column 5: TE field amplitude
# Column 6: TM field amplitude 

set term png size 800,600 enhanced font "Times New Roman,15"

#options / filters
ifile="slab.csv"
ofile="slab.png"
wavelength=1.55
width=0.5
mode=0

set output ofile

set title	"Polarized Mode"
set xlabel	"Position [um]"
set ylabel	"Field Amplitude"
set grid
set autoscale fix
set border 1+2 back

set style fill transparent solid 0.2 noborder
set object 1 rectangle from 0, graph 0 to width, graph 1 \
	fc rgb "gray" fs solid 0.2 behind
	set label "core" at 0.1, graph 0.1 center  tc rgb "black"


plot ifile using (($1==wavelength && $2==width && $3==mode) ? $4 : 1/0):(($1==wavelength && $2==width && $3==mode) ? $5 : 1/0) \
with lines linecolor rgb "red" title "TE" \
,"" using (($1==wavelength && $2==width && $3==mode) ? $4 : 1/0):(($1==wavelength && $2==width && $3==mode) ? $6 : 1/0) \
with lines linecolor rgb "blue" title "TM"

