#ifndef __LIB_EIM_H__
#define __LIB_EIM_H__
/**
 * \brief Effective Index Method Library.
 * \file libeim.h Effective Index Method Algorithms and Utilities Definitions
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

#include <cmath>
#include <libvec.h>
#include <async.h>
#include <libopt.h>
#include <complex>

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

namespace eim
{
	static constexpr double pi = M_PI;	
	static constexpr double eps0 = 8.854188E-12; // Farads per meter (F/m)
	static constexpr double mu0 = 4 * pi * 1E-7; // Henries per meter (H/m)
	static constexpr double c = 1/sqrt(eps0*mu0); // free space speed of light
	static constexpr double eta0 = sqrt(mu0/eps0); // free space impedance

	using field_t = std::complex<double>;

	enum Mode:uint8_t
	{
		TE,
		TM
	};

	/**
	 * \brief Characteristic equation of the 3 layer slab.
	 * This function may be minimized against the input neff using an optimization algorithm.
	 * 
	 * \param n1 box refractive index
	 * \param n2 core refractive index  
	 * \param n3 cladding refractive index  
	 * \param lambda Wavelength in meter
	 * \param W extent of core; slab thickness
	 * \param j the mode order to solve 
	 * \param neff the effective refractive index
	 * \returns the difference between the left and right hand sides of the characteristic equations.
	 */ 
	template<Mode mode>
	double slab_equation(double n1, double n2, double n3, double lambda, double W, int j, double neff)
	{
		double k0 = 2*pi*(1 / lambda); 
		double gamma1 = k0*sqrt( pow(neff, 2) - pow(n1, 2) );
		double gamma2 = k0*sqrt( pow(n2, 2) - pow(neff, 2) );
		double gamma3 = k0*sqrt( pow(neff, 2) - pow(n3, 2) );

		double lhs = gamma2 * W;
		if constexpr (mode == TE)
		{
			double rhs = -atan2(gamma2, gamma1) - atan2(gamma2, gamma3) + (j+1)*pi;
			return rhs - lhs;
		}
		if constexpr (mode == TM)
		{
			double rhs = -atan2(pow(n1, 2) * gamma2, pow(n2, 2) * gamma1) - 
				atan2( pow(n3, 2) * gamma2, pow(n2, 2) * gamma3 ) + (j+1)*pi;
			return rhs - lhs;
		}
	}

	/**
	 * \brief Calculate the fields of 3 layer slab
	 * \param n1 box refractive index
	 * \param n2 core refractive index  
	 * \param n3 cladding refractive index  
	 * \param lambda Wavelength in meter
	 * \param W extent of core; slab thickness
	 * \param j the mode order to solve 
	 * \returns the effective refractive index 
	 */ 
	std::tuple<double, double> 
	solve_slab(double n1, double n2, double n3, double lambda, double W, int j)
	{
		auto slab_TE = [&n1, &n2, &n3, &lambda, &W, &j](double neff) {
			return slab_equation<TE>(n1, n2, n3, lambda, W, j, neff);
		};

		auto slab_TM = [&n1, &n2, &n3, &lambda, &W, &j](double neff) {
			return slab_equation<TM>(n1, n2, n3, lambda, W, j, neff);  
		};

		#if PAR > 0
			auto te_mode = call_async([&slab_TE, &n1, &n2, &n3]() {
				opt::Status s;
				auto nmin = std::min(n1, n3);
				auto n = opt::bisection(slab_TE, nmin, n2, s);
				return ( (s.status == opt::CONVERGED) ? n : nmin );

			});
			
			auto tm_mode = call_async([&slab_TM, &n1, &n2, &n3]() {
				opt::Status s;
				auto nmin = std::min(n1, n3);
				auto n = opt::bisection(slab_TM, nmin, n2, s);
				return ( (s.status == opt::CONVERGED) ? n : nmin );
			});
			return std::make_tuple(te_mode.get(), tm_mode.get());
		#else
			opt::Status s_TE, s_TM;
			auto nmin = std::min(n1, n3);
			auto n_TE = opt::bisection(slab_TE, nmin, n2, s_TE);	
			auto n_TM = opt::bisection(slab_TM, nmin, n2, s_TM);

			return std::make_tuple(( (s_TE.status == opt::CONVERGED) ? n_TE : nmin ), 
			( (s_TM.status == opt::CONVERGED) ? n_TM : nmin ));
		#endif
	}

	/**
	 * \brief Return the mode profile for the TE or TM mode, for the dimension x.
	 * 
	 * The coordinate system is such that x = 0 at the first boundary
	 * y
	 * ^
	 * | n1 | n2 | n3
	 * |----0----W---> x
	 * 
	 * \param x the positions along the slab refractive index profile
	 * \param A the field amplitude of the slab along the transverse dimension
	 * \param Bl the field amplitude of the slab along the lateral dimension
	 * \param Bn the field amplitude of the slab along the normal dimension
	 * \param neff the effective refractive index for the slab
	 * \param n1 box refractive index
	 * \param n2 core refractive index  
	 * \param n3 cladding refractive index  
	 * \param lambda Wavelength in meter
	 * \param W extent of core; slab thickness
	 * \param j the mode order to solve
	 * 
	 * \returns the field orthogonal to the transverse mode. If mode is TE, returns Hy. 
	 * 	
	 **/
	template <Mode mode, typename I1, typename I2>
	void
	mode_1D(I1 x1, I2 x2, field_t* A, field_t* Bl, field_t* Bn,
		double neff, double n1, double n2, double n3, double lambda, double W, int j)
	{
		const size_t xs = std::distance(x1, x2);
		const auto x = &(*x1);

		double k0 = 2*pi*(1 / lambda); 
		double gamma1 = k0*sqrt( pow(neff, 2) - pow(n1, 2) );
		double gamma2 = k0*sqrt( pow(n2, 2) - pow(neff, 2) );
		double gamma3 = k0*sqrt( pow(neff, 2) - pow(n3, 2) );
		
		double alpha =  -atan2(\
			gamma1 * ((mode == TE) ? 1.0 : pow(n2, 2)), 
			gamma2 * ((mode == TE) ? 1.0 : pow(n1, 2))) + j*pi;

		// Applying the Boundary Conditions to the electric and magnetic field equations
		// At x=0:	C1 = C2 * cos(alpha)
		//			-gamma1*C1 = gamma2*C2*sin(alpha)
		// At x=W:	C3 = C2 * cos(gamma2*W + alpha)
		//			-gamma2*C2*sin(gamma2*W + alpha) = -gamma3*C3
		//
		// \note if the coordinate system were taken as x = 0 in the center of the core
		// then the phase shift alpha could be dropped, simplifying the calculation of the coefficients
		// \note Since the equations are homogeneous, the solution is satisfied for N*{C1, C2, C3}, with any factor N.
		// In this case the solution can be set by choosing a scale, N, and setting C2 = N

		double C2 = 1.0;
		double C1 = C2 * cos(alpha) * \
			( (mode == TM) ? pow(n2, 2) / pow(n1, 2) : 1.0 );
		double C3 = C2 * cos(gamma2 * W + alpha) * \
			( (mode == TM) ? pow(n2, 2) / pow(n3, 2) : 1.0 );

		#if 0 // this is for field normalization if desired in the future
		// Region 1: x from -∞ to 0: 
		// $A_1 = \int_{-\inf}^{0} |C_1 e^(\gamma_1 x)|^2!~dx = C_1^2 / (2 gamma_1)$
		double A1 = pow(C1, 2) / (2.0 * gamma1);
		// Region 2: x from 0 to W:
		// $A_2 = \int_0^W cos^2(\gamma_2~x + \alpha)~dx$
		double A2 = pow(C2, 2) * (W / 2.0 + (sin(2 * gamma2 * W + 2 * alpha) - sin(2 * alpha)) / (4.0 * gamma2));
		// Region 3: x from W to +∞:
		// $A_3 = \int_W^\inf |C_3 e^(-\gamma_3*(x-W))|^2~dx = C_3^2 / (2*\gamma_3)$
		double A3 = pow(C3, 2) / (2.0 * gamma3);
		#endif


		auto calculate_field = [&](size_t& i)
		{
			constexpr std::complex<double> j(1.0, 1.0);

			if (x[i] < 0)
			{
				A[i] = C1 * exp(gamma1 * x[i]);

				Bl[i] = (mode == TE) ?
					A[i] * n1 / eta0 :
					A[i] * eta0 / n1;

				Bn[i] = ((mode == TE) ?
					(-gamma1 * A[i]) / (j * 2.*pi*c/lambda *mu0) :
					(gamma1 * A[i] ) / (j * 2.*pi*c/lambda * eps0 * pow(n1, 2)));
			}
			else if (x[i] >= 0 && x[i] <= W)
			{
				A[i] = C2 * cos(gamma2 * x[i] + alpha);

				Bl[i] = (mode == TE) ?
					A[i] * n1 / eta0 :
					A[i] * eta0 / n1;

				auto normal = C2 * gamma2 * sin(gamma2 * x[i] + alpha);
				Bn[i] = ((mode == TE) ? normal /(j * 2.*pi*c/lambda *mu0) :
					-normal  / (j * 2.*pi*c/lambda * eps0 * pow(n2, 2)));
			}
			else if (x[i] > W)
			{
				A[i] = C3 * exp(-gamma3 * (x[i] - W));

				Bl[i] = (mode == TE) ?
					A[i] * n1 / eta0 :
					A[i] * eta0 / n1;

				Bn[i] = ((mode == TE) ?
					(gamma3 * A[i]) / (j * 2.*pi*c/lambda *mu0) :
					(gamma3 * A[i] ) / (j * 2.*pi*c/lambda * eps0 * pow(n3, 2)));
			}
		};

		#if PAR == 1
		{
			// C++ supports parallel execution policies but under the hood it depends on libtbb 
			std::for_each(std::execution::par, x1, x2, [&](const double& xi) 
			{
				//Requires that the storage container is contiguous
				//xi must be passed by reference
				size_t i = &xi - &x[0];
				calculate_field(i);
			});
		}
		#elif PAR == 2
		{
			// home-brewed version of the parallel execution policy with no external dependency
			async_for_each(x1, x2, [&](const double& xi) 
			{
				//Requires that the storage container is contiguous
				//xi must be passed by reference
				size_t i = &xi - &x[0];
				calculate_field(i);
			});
		}
		#else
		{
			//standard serial execution of the calculation
			for(size_t i = 0; i < xs; i++)
			{
				calculate_field(i);
			}
		}
		#endif
	}

}
#endif //__LIB_EIM_H__