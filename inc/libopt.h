#ifndef __LIB_OPT_H__
#define __LIB_OPT_H__
/**
 * \brief Optimization Library.
 * \file libopt.h Optimization Utilities Definitions
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
#include <limits>
#include <functional>
#include <iostream>

namespace opt
{
	enum : uint8_t
	{
		CONVERGED,
		DIVERGED,
		INVALID_RANGE
	};
	/**
	 * \brief Status optimization run statistics.
	 **/
	struct Status
	{
		uint8_t status; ///< Indicates if run Converged or Diverged
		uint32_t iterations; ///< Number of iterations used
		double residual; ///< residual at final run 
	}__attribute__((packed));

	/**
	 * \brief Bisection method to find the root of a function.
	 *
	 * This function uses the bisection method to approximate the root of a function
	 * within a given interval [a, b]. The method iterates until the desired tolerance
	 * or the maximum number of iterations is reached.
	 *
	 * \param func The function whose root is sought, represented as a std::function.
	 * \param a The left boundary of the interval.
	 * \param b The right boundary of the interval.
	 * \param tol Tolerance for the root approximation (default is 1e-6).
	 * \param max_iter Maximum number of iterations (default is 100).
	 * \return The approximated root or NaN if convergence fails.
	 */
	double 
	bisection(const std::function<double(double)>& f, 
			 double a, double b, Status& s, const double tol = 1e-4, const int max_iter = 100) 
	{
		double fa = f(a);
		double fb = f(b);

		if (fa * fb > 0) 
		{
			s.status = INVALID_RANGE;
			s.iterations = 0;
			s.residual = std::min(std::fabs(fa), std::fabs(fb));
			return a;
		}

		double midpoint, fmid;
		int iter = 0;
		double* pa = &a, *pb = &b, *pmid = &midpoint;

		*pmid = (*pa + *pb) / 2.0;
		fmid = f(*pmid);
		
		while (*pmid > tol && std::fabs(fmid) > tol && iter < max_iter)
		{
			*pmid = (*pa + *pb) / 2.0;
			fmid = f(*pmid);
			
			if (std::fabs(fmid) < tol) 
			{
				s.status = CONVERGED;
				s.iterations = iter;
				s.residual = std::fabs(fmid);
				return *pmid;
			}

			if (fa * fmid < 0) 
			{
				*pb = *pmid;
				fb = fmid;
			} 
			else 
			{
				*pa = *pmid;
				fa = fmid;
			}
			iter++;
		}

		*pmid = (*pa + *pb) / 2.0;
		fmid = f(*pmid);

		s.iterations = iter;
		s.residual = std::fabs(fmid);

		if ((*pb - *pa) / 2.0 <= tol) 
		{
			s.status = CONVERGED;
		} 
		else 
		{
			s.status = DIVERGED;
		}

		if (std::fabs(*pmid - b) < tol || std::fabs(*pmid - a) < tol) 
		{
			s.status = DIVERGED;
		}

		return *pmid;
	}

}
#endif //__LIB_OPT_H__