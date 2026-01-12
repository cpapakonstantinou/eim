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

#include <iostream>
#include <numeric>
#include <log.h>
#include <eim.h>
#include <ctl.h>
#include <strip.h>
#include <slot.h>

using namespace std;
using namespace eim;

const char* usage = 
	"usage: eim [opts]\n"
	"\nWaveguide Control:\n"
	"\t-t <type>               Waveguide type: 'strip' or 'slot'\n"
	"\t-r <thickness>          Rib/core thickness\n"
	"\t-s <thickness>          Slab thickness\n"
	"\t-w <width>[,...]        Rib/core width(s)\n"
	"\t-S <width>              Slot width\n"
	"\t-n <n_box>,<n_core>,<n_clad>[,<n_slot>] Refractive indices\n"
	"\t-m <mode>               Mode polarization: 'TE' or 'TM'.\n"
	"\t-j <order>[,...]        Mode order(s): 0,1,2,...\n"
	"\t-l <wavelength>[,...]   Wavelength\n"
	"\nOutput Control:\n"
	"\t-O                      Enable 2D mode field calculation\n"
	"\t-o <filename>           Output filename for mode field\n"
	"\t-e <extent>             Spatial extent for field calculation\n"
	"\t-p <points>             Number of points per axis\n";

int main(int argc, char* argv[])
{
	std::unique_ptr<ctl> ctx = std::make_unique<ctl>(); //Application Control struct

	try // Parsing command line
	{
		int c;
		while ((c = getopt(argc, argv, "e:j:hl:m:n:o:Op:r:s:S:t:w:")) != -1) 
		{
			switch (c) 
			{
				case 't':
				{
					string device{optarg};
					if (device == "strip") 
					{
						ctx->device = Waveguide::STRIP;
					} 
					else if (device == "slot") 
					{
						ctx->device = Waveguide::SLOT;
					} 
					else 
					{
						cerr << "[ERROR] waveguide type: must be 'strip' or 'slot'." << endl;
						return -1;
					}
					break;
				}
				case 'e':
				{
					ctx->extent = stod(optarg);
					break;
				}
				case 'j':
				{
					parse_numeric<unsigned>(optarg, ctx->mode_orders);
					break;
				}
				case 'l':
				{
					parse_numeric<double>(optarg, ctx->wavelengths);
					break;
				}
				case 'm':
				{
					string str{optarg};
					if (str != "TE" && str != "TM") 
					{
						cerr << "[ERROR] mode: must be 'TE' or 'TM'." << endl;
						return -1;
					}
					ctx->mode = ((str == "TE") ? TE : TM);
					break;
				}
				case 'n':
				{
					char* end = NULL;
					const char* str = optarg;
					
					// Parse n_box
					ctx->n_box = strtod(str, &end);
					if (str == end) 
					{
						cerr << "[ERROR] n: n_box missing" << endl;
						return -1;
					}
					str = (*end) ? end + 1 : end;

					// Parse n_core
					ctx->n_core = strtod(str, &end);
					if (str == end) 
					{
						cerr << "[ERROR] n: n_core missing" << endl;
						return -1;
					}
					str = (*end) ? end + 1 : end;

					// Parse n_clad
					if (*str == '\0') 
					{
						cerr << "[ERROR] n: n_clad missing" << endl;
						return -1;
					}
					ctx->n_clad = strtod(str, &end);
					if (str == end) 
					{
						cerr << "[ERROR] n: n_clad missing or invalid" << endl;
						return -1;
					}
					
					// Parse n_slot (optional, but required for slot waveguides)
					str = (*end) ? end + 1 : end;
					if (*str != '\0') 
					{
						ctx->n_slot = strtod(str, &end);
						if (str == end) 
						{
							cerr << "[ERROR] n: n_slot invalid" << endl;
							return -1;
						}
					}
					break;
				}
				case 'o':
				{
					ctx->mode_logname = optarg;
					break;
				}
				case 'O':
				{
					ctx->mode_log = true;
					break;
				}
				case 'p':
				{
					ctx->pts = stoul(optarg);
					break;
				}
				case 'r':
				{
					ctx->t_core = stod(optarg);
					break;
				}
				case 's':
				{
					ctx->t_slab = stod(optarg);
					break;
				}
				case 'S':
				{
					parse_numeric<double>(optarg, ctx->gaps);
					break;
				}
				case 'w':
				{
					parse_numeric<double>(optarg, ctx->widths);
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
		cerr << "[ERROR] opts: parsing arguments: " << ex.what() << endl;
		return -1;
	}

	try // validation
	{
		if (ctx->wavelengths.empty()) 
		{
			cerr << "[ERROR] setup: Must specify at least one wavelength" << endl;
			return -1;
		}

		if (ctx->widths.empty()) 
		{
			cerr << "[ERROR] setup: Must specify at least one width" << endl;
			return -1;
		}

		if (ctx->mode_orders.empty()) 
		{
			cerr << "[ERROR] setup: Must specify at least one mode order" << endl;
			return -1;
		}

		if (!ctx->n_core || !ctx->n_clad || !ctx->n_box) 
		{
			cerr << "[ERROR] setup: Must specify refractive index" << endl;
			return -1;
		}

		if (!ctx->t_core) 
		{
			cerr << "[ERROR] setup: Must specify core thickness" << endl;
			return -1;
		}

		if (ctx->device == SLOT && ctx->gaps.empty()) 
		{
			cerr << "[ERROR] setup: Must specify at least one slot width" << endl;
			return -1;
		}

		if(ctx->device != STRIP && ctx->device != SLOT) 
		{
			cerr << "[ERROR] setup: supported devices: 'strip', 'slot'." << endl;
			return -1;
		}

		if(ctx->mode_log) 
		{
			if(!ctx->pts)
			{
				cerr << "[ERROR] setup: Must set number of mode points" << endl;
				return -1;	
			}
			if(!ctx->extent)
			{
				cerr << "[ERROR] setup: Must set mode extent" << endl;
				return -1;	
			}

		}

	}
	catch(const exception& ex)
	{
		cerr << "[ERROR] setup: " << ex.what() << endl;
		return -1;
	}

	try // Running the program
	{
		if (ctx->device == Waveguide::STRIP)
		{
			Strip wg{
				.wavelength = ctx->wavelengths[0],
				.t_rib = ctx->t_core,
				.t_slab = ctx->t_slab,
				.w_rib = ctx->widths[0],
				.w_slab = 0, // Not used in EIM
				.n_box = ctx->n_box,
				.n_core = ctx->n_core,
				.n_clad = ctx->n_clad,
				.mode_order = ctx->mode_orders[0],
				.mode = ctx->mode
			};

			printf("t_slab,t_rib,width,wavelength,mode,neff\n");

			// Calculate neff for each width and mode order
			for(const auto& l : ctx->wavelengths)
			{
				wg.wavelength = l;
				for (const auto& w : ctx->widths)
				{
					wg.w_rib = w;
					for (const auto& j : ctx->mode_orders)
					{
						wg.mode_order = j;
						printf("%.3g,%.3g,%.3g,%.4g,%s%lu,%.6g\n",
							wg.t_slab, 
							wg.t_rib, 
							wg.w_rib,
							wg.wavelength, 
							wg.mode == TE ? "TE" : "TM",
							wg.mode_order, 
							wg());
					}
				}
			}

			// Mode field calculation
			if (ctx->mode_log)
			{
				if (!ctx->mode_logname)
					ctx->mode_logname = "mode2D_strip.csv";

				Log mode2D(ctx->mode_logname, ",");
				mode2D << "t_slab" << "t_rib" << "width" << "mode" 
					   << "transverse" << "lateral" << "amplitude";
				++mode2D;

				cvector<double> x(ctx->pts);
				vec::linspace(x.begin(), x.end(), -ctx->extent, ctx->extent);
				cmatrix<field_t> field(ctx->pts, ctx->pts);

				auto log_mode = [&]()
				{
					wg.mode_2D(x, field);
					for (size_t i = 0; i < ctx->pts; ++i) 
					{
						for (size_t j = 0; j < ctx->pts; ++j)
						{
							mode2D << wg.t_slab << wg.t_rib << wg.w_rib
								   << ((wg.mode == TE ? "TE" : "TM") + to_string(wg.mode_order))
								   << x[i] << x[j] << abs(field[i][j]);
							++mode2D;
						}
					}
				};

				// Calculate fields for all width/mode combinations
				for(const auto& l : ctx->wavelengths)
				{
					wg.wavelength = l;
					for (const auto& w : ctx->widths)
					{
						wg.w_rib = w;
						for (const auto& j : ctx->mode_orders)
						{
							wg.mode_order = j;
							log_mode();
						}
					}
				}
			}
		}
		else // SLOT waveguide
		{
			waveguide wg{
				.wavelength = ctx->wavelengths[0],
				.t_core = ctx->t_core,
				.w_core = ctx->widths[0],
				.w_slot = ctx->gaps[0],
				.n_box = ctx->n_box,
				.n_clad = ctx->n_clad,
				.n_core = ctx->n_core,
				.n_slot = ctx->n_slot,
				.mode_order = ctx->mode_orders[0],
				.mode = ctx->mode
			};

			printf("t_core,w_core,w_slot,wavelength,mode,neff\n");

			// Calculate neff for each width, gap and mode order
			for(const auto& l : ctx->wavelengths)
			{
				wg.wavelength = l;
				for (const auto& g: ctx->gaps)
				{
					wg.w_slot = g;
					for (const auto& w : ctx->widths)
					{
						wg.w_core = w;
						for (const auto& j : ctx->mode_orders)
						{
							wg.mode_order = j;
							printf("%.3g,%.3g,%.3g,%.4g,%s%lu,%.6g\n",
								wg.t_core, 
								wg.w_core, 
								wg.w_slot,
								wg.wavelength, 
								wg.mode == TE ? "TE" : "TM",
								wg.mode_order, 
								wg());
						}
					}
				}
			}

			// TODO: 2D mode field calculation not yet implemented for slot
			if (ctx->mode_log)
			{
				cerr << "[WARN]: 2D mode field calculation not implemented for slot waveguides." << endl;
			}
		}
	}
	catch(const exception& ex)
	{
		cerr << "[ERROR] calculation: " << ex.what() << endl;
		return -1;
	}

	return 0;
}