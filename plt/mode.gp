set term png size 800,600 enhanced font "Times New Roman,15"

# Options / Filters
ifile="mode2D.csv"
ofile="mode2D.png"
mode="TE0"
width="0.5"
tmpfile = "mode-" . mode . "_w-" . width . ".dat"

set output ofile
set palette rgbformulae 34,35,36

# Filter the data using awk
system("awk -F, '{if (($1+0 == " . width . ") && ($2 == \"" . mode . "\")) print $1, $2, $3, $4, $5}' " . ifile . " > " . tmpfile)

# Plot settings
set xlabel "X (um)"
set ylabel "Y (um)"
set title "Effective Index Method"
set size ratio -1  # Aspect ratio 1:1 (equal axes)

set pm3d map
set view map
set autoscale fix

# Plot the filtered data
splot tmpfile using 3:4:5 with image
