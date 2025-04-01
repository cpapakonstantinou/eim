/**
 * \brief Effective Index Method.  
 * \file eim.cc Command Line Interface to effective index method 
 * \author c. papakonstantinou
 */
// Copyright (c) 2025  Constantine Papakonstantinou
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <libeim.h>
#include <iostream>
#include <log.h>
#include <numeric>
#include <list>
#include <carray.h>

//control parallel calculation of loops
// PAR == 1 uses std::execution
// PAR == 2 uses async.h
// PAR == 0 is sequential
#ifndef PAR
	#define PAR 0
#endif //PAR

#if PAR == 1
	#include <execution>
#endif

using namespace std;
using namespace eim;


/**
 * \brief functor for waveguide geometry.
 * The 2D waveguide geometry is specified by construction of the structure
 * The () operator is overloaded to solve the waveguide by the effective index method
 * \note the w_slab parameter is not used for the effective index method
 * 
 * y
 * ↑
 * → x
 *                <w_slab> <w_rib>  <w_slab>
 *                -------- -------- --------
 *               | n_clad | n_clad |  ...   |    
 *                -------- -------- --------
 * t_slab, t_rib | n_core | n_core |  ...   | 
 *                -------- -------- --------
 *               | n_box  | n_box  |  ...   |
 * 		  
 **/
struct Waveguide
{
	double wavelength; ///< wavelength
	double t_rib; ///< thickness of the rib layer
	double t_slab; ///< thickness of the slab layer
	double w_rib; ///< width of the rib layer 
	double w_slab; ///< width of the slab layer
	double n_box; ///< refractive index of buried oxide layer
	double n_core; ///< refractive index of core
	double n_clad; ///< refractive index of cladding
	size_t mode_order; ///< index of the mode
	Mode mode; ///< TE or TM mode

	/**
	 * \brief calculate the effective refractive index
	 * \returns the effective refractive index 
	 **/
	double 
	operator()()
	{
		
		auto n1 = solve_slab(n_box, (t_slab ? n_core : n_clad), n_clad, wavelength, t_slab, 0);
		auto n2 = solve_slab(n_box, n_core, n_clad, wavelength, t_rib, 0);
		const auto& n3 = n1;

		// The TE mode of the waveguide is the TM mode of the analysis
		// For TM mode analysis, it is the opposite order
		if (mode ==  TE)
		{
			auto neff = solve_slab(get<0>(n1), get<0>(n2), get<0>(n3), wavelength, w_rib, mode_order);
			return get<1>(neff);
		}
		else //(mode == TM)
		{
			auto neff = solve_slab(get<1>(n1), get<1>(n2), get<1>(n3), wavelength, w_rib, mode_order);
			return get<0>(neff);
		}
	}
	/**
	 * \brief calculate the mode field amplitude
	 * \param x the span of points in x and y to calculate the field amplitude for
	 * \param A the field amplitude
	 **/
	void 
	mode_2D(cvector<double>& x, cmatrix<field_t>& field)
	{
		size_t N = std::distance(x.begin(), x.end());
		//Vertical field amplitude
		cvector<field_t> E_slab ( N );
		cvector<field_t> H_slab ( N );
		cvector<field_t> _ ( N );

		cvector<field_t> E_wg( N );
		cvector<field_t> H_wg( N );
		
		if (mode == TE)
		{	
			auto n1 = solve_slab(n_box, (t_slab ? n_core : n_clad) , n_clad, wavelength, t_slab, mode_order); 
			auto n2 = solve_slab(n_box, n_core , n_clad, wavelength, t_rib, mode_order);
			const auto& n3 = n1;
			mode_1D<TE>(x.begin(), x.end(), E_slab.begin(), H_slab.begin(), _.begin(), get<0>(n2), n_box, n_core, n_clad, wavelength, t_rib, mode_order);
			auto neff = solve_slab(get<0>(n1), get<0>(n2), get<0>(n3), wavelength, w_rib, mode_order);
			mode_1D<TM>(x.begin(), x.end(), H_wg.begin(), E_wg.begin(), _.begin(),  get<1>(neff), get<0>(n1), get<0>(n2), get<0>(n3), wavelength, w_rib, mode_order);
			
			if constexpr (PAR == 0)
				vec::outer_product<field_t>(H_wg.begin(), H_wg.end(), E_slab.begin(), E_slab.end(), &field[0]);
			else
				vec::async_outer_product<field_t>(H_wg.begin(), H_wg.end(), E_slab.begin(), E_slab.end(), &field[0]);
		}
		else // (mode == TM)
		{
			auto n1 = solve_slab(n_box, (t_slab ? n_core : n_clad) , n_clad, wavelength, t_slab, mode_order); 
			auto n2 = solve_slab(n_box, n_core , n_clad, wavelength, t_rib, mode_order);
			const auto& n3 = n1;
			mode_1D<TM>(x.begin(), x.end(), H_slab.begin(), E_slab.begin(), _.begin(), get<1>(n2), n_box, n_core, n_clad, wavelength, t_rib, mode_order);
			auto neff = solve_slab(get<1>(n1), get<1>(n2), get<1>(n3), wavelength, w_rib, mode_order);
			mode_1D<TE>(x.begin(), x.end(), E_wg.begin(), H_wg.begin(), _.begin(), get<0>(neff), get<1>(n1), get<1>(n2), get<1>(n3), wavelength, w_rib, mode_order);
			
			if constexpr (PAR == 0)
				vec::outer_product<field_t>(E_wg.begin(), E_wg.end(), H_slab.begin(), H_slab.end(), &field[0]);
			else
				vec::async_outer_product<field_t>(E_wg.begin(), E_wg.end(), H_slab.begin(), H_slab.end(), &field[0]);
		}


	}
};

const char* usage = "usage eim [opts]\n" 
					"-e extents of axes in mode\n"
					"-j mode order 0,1,2,...,N\n"
					"-l wavelength\n"
					"-m TE|TM\n"
					"-n n_box,n_core,n_clad\n"
					"-N number of points for axis extent in mode\n"
					"-o name of field amplitude log\n"
					"-O output log of field amplitude for mode(s)"
					"-r rib_thickness\n"
					"-s slab_thickness\n"
					"-w rib_width\n";

int main(int argc, char* argv[])
{
	const char* mode_logname = NULL;
	bool mode_log = false;

	Waveguide wg 
	{
		.wavelength = 1.55,
		.t_rib = 0.220,
		.t_slab = 0,
		.w_rib = 0.5,
		.w_slab = 0,
		.n_box = 1.44,
		.n_core = 3.47,
		.n_clad = 1.44,
		.mode_order = 0,
		.mode = TE
	};

	std::list<size_t> mode_orders;
	std::list<double> widths;
	size_t pts = 100; 
	double extent = 1;
	try //configuring the program
	{
		int c;
		while ((c = getopt(argc, argv, "e:j:hl:m:n:o:Op:r:s:w:")) != -1) 
		{
			switch (c) 
			{
				case 'e':
				{
					extent = stod(optarg);
					break;
				}
				case 'j':
				{
					char* end;
					const char* str = optarg;
					while (*str) 
					{
						size_t val = strtoul(str, &end, 10);
						if (str == end) break;
						mode_orders.push_back(val);
						str = (*end) ? end + 1 : end;
					}
					
					break;
				}
				case 'l':
				{
					wg.wavelength = stod(optarg);
					break;
				}
				case 'm':
				{
					string str{optarg};
					if (str != "TE" && str != "TM") 
					{
						cerr << "Invalid mode. Use \"TE\" or \"TM\"." << endl;
						return -1;
					}
					wg.mode = ((str == "TE") ? TE : TM);
					break;
				}
				case 'n':
				{
					char* end = NULL;
					const char* str = optarg;
					
					wg.n_box = strtod(str, &end);
					if (str == end) 
					{
						cerr << "n_box missing" << endl;
						return -1;
					}
					str = (*end) ? end + 1 : end;

					// Parse n_core
					wg.n_core = strtod(str, &end);
					if (str == end) 
					{
						cerr << "n_core missing" << endl;
						return -1;
					}
					str = (*end) ? end + 1 : end;

					// Parse n_clad
					if (*str == '\0') 
					{
						cerr << "n_clad missing" << endl;
						return -1;
					}
					wg.n_clad = strtod(str, &end);
					if (str == end) 
					{
						cerr << "n_clad missing or invalid" << endl;
						return -1;
					}

					break;
				}
				case 'o':
				{
					mode_logname = optarg;
					break;
				}
				case 'O':
				{
					mode_log = true;
					break;
				}
				case 'p':
				{
					pts = stoul(optarg);
					break;
				}
				case 'r':
				{
					wg.t_rib = stod(optarg);
					break;
				}
				case 's':
				{
					wg.t_slab = stod(optarg);
					break;
				}
				case 'w':
				{
					char* end = NULL;
					const char* str = optarg;
					while (*str) 
					{
						double val = strtod(str, &end);
						if (str == end) break;
						widths.push_back(val);
						str = (*end) ? end + 1 : end;
					}
					break;
				}
				case 'h':
				default:
					cerr << usage << endl;
					return -1;
			}
		}

	}
	catch(const exception& ex)
	{
		cerr << ex.what() << endl;
		return -1;
	}

	try //running the program
	{
		printf("width,mode,neff\n");
		if( !widths.empty() )
		{
			for (const auto& w : widths)
			{
				wg.w_rib = w;
				if( !mode_orders.empty() )
				{
					for(const auto& j : mode_orders)
					{
						wg.mode_order = j;
						printf("%.3g,%s%lu,%g\n",wg.w_rib, wg.mode == TE ? "TE" : "TM", wg.mode_order, wg());
					}
				}
				else printf("%.3g,%s%lu,%g\n",w, wg.mode == TE ? "TE" : "TM", wg.mode_order, wg());
			}
		}
		else printf("%.3g,%s%lu,%g\n",wg.w_rib, wg.mode == TE ? "TE" : "TM", wg.mode_order, wg());

		if(!mode_logname)
			mode_logname = "mode2D.csv";
		if (mode_log)
		{
			Log mode2D( mode_logname, ",");
			mode2D << "width" << "mode" << "transverse" << "longitudinal" << "amplitude";
			++mode2D;

			cvector<double> x (pts);

			vec::linspace(x.begin(), x.end(), -extent, extent);

			cmatrix<field_t> field( pts, pts );

			auto log_mode = [&wg, &pts, &x, &field, &mode2D]()
			{
				wg.mode_2D(x, field);
				//iterate in row major
				for (size_t i = 0; i < pts; ++i) 
				{
					for (size_t j = 0; j < pts; ++j)
					{
						mode2D << wg.w_rib 
						<< ( (wg.mode == TE ? "TE" : "TM") + to_string(wg.mode_order) )
						<< x[i] << x[j] 
						<< abs( field[i][j] );
						++mode2D;
					}
				}
			};

			if( !widths.empty() )
			{
				for (const auto& w : widths)
				{
					wg.w_rib = w;
					if( !mode_orders.empty() )
					{
						for(const auto& j : mode_orders)
						{
							wg.mode_order = j;
							log_mode();
						}
					}
					else log_mode();
				}
			}
			else log_mode();
		}
	}
	catch(const exception& ex)
	{
		cerr << ex.what() << endl;
		return -1;
	}
	return 0;
}