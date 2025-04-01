set term png size 800,600 enhanced font "Times New Roman,15"

ifile="mode2D.csv"
ofile="mode2D.png"
mode="TE0"
width="0.5"
t_rib="0.22"
t_slab="0"
tmpfile =  "t_slab-" . t_slab ."_t_rib-" . t_rib ."_w-" . width . "_mode-" . mode . ".dat"

set output ofile
set palette rgbformulae 34,35,36

system("awk -F, '{if (($3+0 == " . width . ") && ($4 == \"" . mode . "\")) print $1, $2, $3, $4, $5, $6, $7}' " . ifile . " > " . tmpfile)

# Plot settings
set xlabel "X (um)"
set ylabel "Y (um)"
set title mode
set size ratio -1  

set pm3d map
set view map
set autoscale fix

splot tmpfile using 5:6:7 with image
