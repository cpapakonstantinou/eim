set terminal pngcairo size 800,600 enhanced font "Times New Roman,15"

# Input and output file settings
ifile="eim.csv"
ofile="eim.png"

# Temporary files for filtered data
file_TE0 = "TE0.dat"
file_TE1 = "TE1.dat"
file_TE2 = "TE2.dat"

# Filter the data for each mode and save to temporary files
system('grep "TE0" ' . ifile . ' > ' . file_TE0)
system('grep "TE1" ' . ifile . ' > ' . file_TE1)
system('grep "TE2" ' . ifile . ' > ' . file_TE2)

# Set data file separator to comma
set datafile separator ","

# Output file
set output ofile

# Set labels and title
set xlabel 'Waveguide Width (um)'
set ylabel 'Effective Index'
set autoscale xfixmax
set autoscale yfixmax
set border 1+2 back
# Set grid and style
set grid
# Plot the data
plot file_TE0 using 1:3 with linespoints title 'TE0' pt 5 lc rgb "red", \
	 file_TE1 using 1:3 with linespoints title 'TE1' pt 5 lc rgb "black", \
	 file_TE2 using 1:3 with linespoints title 'TE2' pt 5 lc rgb "pink"

# Clean up temporary files
system('rm ' . file_TE0)
system('rm ' . file_TE1)
system('rm ' . file_TE2)
