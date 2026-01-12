#ifndef __SLOT_H__
#define __SLOT_H__

/**
 * \brief Slot waveguide utilities.
 * \file slot.h Slot Waveguide Mode Equations
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

#include <libopt.h>
#include <libvec.h>
#include <eim.h>
#include <strip.h>

namespace eim
{
	/**
	 * \brief Five-layer slot waveguide characteristic equation (cosh-type even mode)
	 * This is for the case n_core > n_clad >= n_slot
	 * 
	 * \param n_clad cladding refractive index
	 * \param n_core core refractive index  
	 * \param n_slot slot refractive index  
	 * \param lambda Wavelength in meter
	 * \param a half-width of slot
	 * \param b half-width of slot + core thickness
	 * \param j mode order
	 * \param neff effective refractive index
	 * \returns difference between LHS and RHS of characteristic equation
	 */ 
	double slot_cosh_equation(double n_clad, double n_core, double n_slot, 
							  double lambda, double a, double b, int j, double neff)
	{
		double k0 = 2*pi / lambda; 
		
		// For neff between n_clad and n_core
		double gamma_slot = k0*sqrt(neff*neff - n_slot*n_slot);
		double kappa_core = k0*sqrt(n_core*n_core - neff*neff);
		double gamma_clad = k0*sqrt(neff*neff - n_clad*n_clad);
		
		double term1 = atan2(n_core*n_core * gamma_clad, n_clad*n_clad * kappa_core);
		double term2 = atan2(n_core*n_core * gamma_slot * tanh(gamma_slot * a), 
							n_slot*n_slot * kappa_core);
		double lhs = term1 + term2 + (j)*pi;
		double rhs = kappa_core * (b - a);
		
		return rhs - lhs;
	}

	/**
	 * \brief Five-layer slot waveguide characteristic equation (sinh-type odd mode)
	 */ 
	double slot_sinh_equation(double n_clad, double n_core, double n_slot, 
							 double lambda, double a, double b, int j, double neff)
	{
		double k0 = 2*pi / lambda; 
		
		double gamma_slot = k0*sqrt(neff*neff - n_slot*n_slot);
		double kappa_core = k0*sqrt(n_core*n_core - neff*neff);
		double gamma_clad = k0*sqrt(neff*neff - n_clad*n_clad);
		
		// coth(x) = 1/tanh(x)
		double coth_term = 1.0 / tanh(gamma_slot * a);
		
		double term1 = atan2(n_core*n_core * gamma_clad, n_clad*n_clad * kappa_core);
		double term2 = atan2(n_core*n_core * gamma_slot * coth_term, 
							n_slot*n_slot * kappa_core);
		double lhs = term1 + term2 + (j)*pi;
		double rhs = kappa_core * (b - a);
		
		return rhs - lhs;
	}

	/**
	 * \brief Solve 5-layer symmetric slot waveguide for both even and odd modes
	 * 
	 * \param n_clad cladding refractive index
	 * \param n_core core refractive index  
	 * \param n_slot slot refractive index  
	 * \param lambda Wavelength in meter
	 * \param w_slot slot width (= 2*a)
	 * \param w_core core thickness (= b - a)
	 * \param j mode order
	 * \returns tuple of (neff_cosh, neff_sinh) for even and odd modes
	 */ 
	std::tuple<double, double> 
	solve_slot_slab(double n_clad, double n_core, double n_slot, 
					double lambda, double w_slot, double w_core, int j)
	{
		double a = w_slot / 2.0;
		double b = a + w_core;
		
		auto cosh_func = [&](double neff) {
			return slot_cosh_equation(n_clad, n_core, n_slot, lambda, a, b, j, neff);
		};

		auto sinh_func = [&](double neff) {
			return slot_sinh_equation(n_clad, n_core, n_slot, lambda, a, b, j, neff);  
		};

		opt::Status s_cosh, s_sinh;
		auto nmin = std::max(n_clad, n_slot);  // Mode must be guided
		
		// Solve for cosh-type (even) mode
		auto n_cosh = opt::bisection(cosh_func, nmin, n_core, s_cosh);
		
		// Solve for sinh-type (odd) mode
		auto n_sinh = opt::bisection(sinh_func, nmin, n_core, s_sinh);

		return std::make_tuple(
			(s_cosh.status == opt::CONVERGED) ? n_cosh : nmin,
			(s_sinh.status == opt::CONVERGED) ? n_sinh : nmin
		);
	}

	/**
	 * \brief functor for slot waveguide geometry.
	 * The 2D waveguide geometry is specified by construction of the structure
	 * The () operator is overloaded to solve the waveguide by the effective index method
	 * 
	 *  z ^
	 *    |
	 *    +------------------------
	 *    |  clad  | clad  | clad  |  
	 *    +--------+-------+--------
	 *    |  core  | slot  | core  |  <- t_core
	 *    +--------+-------+--------
	 *    |  box   | box   | box   |
	 *    +------------------------
	 *    |        ^       ^
	 *    ---------|-------|------> y
	 *             w_core  w_slot
	 * 
	 * \note First solves vertical 3-layer slabs for each region
	 * \note After solves horizontal 5-layer structure with effective indices
	 */
	struct waveguide
	{
		double wavelength;     ///< wavelength
		double t_core;         ///< thickness/height of core in z direction
		double w_core;         ///< width of core (horizontal slab thickness)
		double w_slot;         ///< width of slot
		double n_box;          ///< refractive index of substrate/box
		double n_clad;         ///< refractive index of top cladding
		double n_core;         ///< refractive index of core
		double n_slot;         ///< refractive index of slot
		size_t mode_order;     ///< mode order
		Mode mode;             ///< TE or TM mode

		/**
		 * \brief calculate the effective refractive index
		 * \returns the effective refractive index 
		 **/
		double operator()()
		{
			//Core refractive index is obtained by 3-layer slabs
			auto neff_core = solve_slab(n_box, n_core, n_clad, wavelength, t_core, 0);
			
			// Slot refractive index obtained by 3-layer slabs
			#if 1
				auto ns = solve_slab(n_box, n_slot, n_clad, wavelength, t_core, 0);
				double neff_slot = (mode == TE) ? std::get<0>(ns) : std::get<1>(ns);
			#endif

			// approximate effective index of slot region with slot index directly
			// use n_slot if it's < min(n_box, n_clad)
			#if 0
				double neff_slot = n_slot;
			#endif

			// Outer cladding regions: box/clad/clad (no core)
			// Calculate their effective index properly
			auto neff_clad = solve_slab(n_box, n_clad, n_clad, wavelength, t_core, 0);

			
			// Solve 5-layer slot structure
			if (mode == TE) // The quasi-TE mode corresponds to TM of the horizontal slab
			{
				auto neff = solve_slot_slab(
					std::get<0>(neff_clad),			// cladding region (TM mode of outer regions)
					std::get<0>(neff_core),			// core region (TM mode of vertical slab)
					neff_slot,		         		// slot region 
					wavelength,
					w_slot,
					w_core,
					mode_order
				);
				return std::get<0>(neff);  // Return cosh-type (even) mode
			}
			else // The quasi-TM mode corresponds to TE of the horizontal slab
			{ 
				auto neff = solve_slot_slab(
					std::get<1>(neff_clad),			// cladding region (TE mode of outer regions)
					std::get<1>(neff_core),			// core region (TE mode of vertical slab)
					neff_slot,						// slot region
					wavelength, 
					w_slot, 
					w_core, 
					mode_order
				);
				return std::get<0>(neff);  // Return cosh-type (even) mode
			}
		}
	};

} // namespace eim

#endif //__SLOT_H__