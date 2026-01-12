#ifndef __EIM_H__
#define __EIM_H__

/**
 * \brief Effective Index Method Constants.
 * \file eim.h
 * \author cpapakonstantinou
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
#include <complex>
#include <vector>
#include <carray.h>

#ifndef PARALLEL
	#define PARALLEL 1
#endif

#if PARALLEL
	#include<execution>
#endif

namespace eim
{
	static constexpr double pi = M_PI;	
	static constexpr double eps0 = 8.854188E-12; // Farads per meter (F/m)
	static constexpr double mu0 = 4 * pi * 1E-7; // Henries per meter (H/m)
	static constexpr double c = 1/sqrt(eps0*mu0); // free space speed of light
	static constexpr double eta0 = sqrt(mu0/eps0); // free space impedance

	using field_t = std::complex<double>;

	/**
	 * \brief Mode type
	 */
	enum Mode:uint8_t
	{
		TE,
		TM
	};

	/**
	 * \brief Waveguide type
	 */
	enum Waveguide:uint8_t
	{
		STRIP,
		SLOT
	};
}//namespace eim

#endif //__EIM_H__