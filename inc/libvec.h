#ifndef __LIB_VEC_H__
#define __LIB_VEC_H__
/**
 * \brief Vector Library.
 * \file libvec.h Vector Algorithms and Utilities Definitions
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

#include <vector>
#include <numeric>
#include <exception>
#include <async.h>

// PAR == 1 uses std::execution
#ifndef PAR
	#define PAR 0
#endif //PAR

#if PAR == 1
	#include <execution>
#endif

namespace vec
{
	 /** 
	 *	\brief A Utility for generating a vector of linearly spaced points  
	 *	\param a beginning value
	 *	\param b end value
	 *	\param begin iterator to store points in range
	 * 	\param end iterator to store points in range
	 * 
	 */
	template <typename T = double, typename I>
	void linspace(I begin, I end, T a, T b) 
	{
		// Calculate the number of elements in the range
		size_t N = std::distance(begin, end);

		if (N <= 1)
			throw std::invalid_argument("The range must contain at least two elements.");

		// Calculate the step size
		T h = (b - a) / static_cast<T>(N - 1); 
		
		T i = 0;
		for (I it = begin; it != end; ++it)
			*it = a + h * (i++);
	}

	/** \brief A Utility for taking the dot product of a vector.
	 * vec::inner product is an alias for std::inner_product
	 */
	using std::inner_product;

	 /** \brief A Utility for calculating the dot product of a vector in parallel.  
	 *	Requires that a and b are contiguous storage.
	 * 	Optionally specify the type (T) of the vector with async_inner_product<T>(...)
	 * 
	 *	\param a1 base address of a
	 *	\param a2 end address of a	
	 *	\param b1 base address of b
	 *	\param b2 end address of b
	 *  \param c scalar storage 	
	 *  
	 */
	template <typename T = double, typename I1, typename I2>
	void 
	async_inner_product(I1 a1, I1 a2, I2 b1, I2 b2, T& c,
			size_t threads = std::thread::hardware_concurrency()) 
	{
		// Guard against invalid input sizes
		if (std::distance(a1, a2) != std::distance(b1, b2)) 
			throw std::invalid_argument("a.size() != b.size()");

		// Guard against redundant calculations
		if (a1 == a2) 
			return T(0);

		#if PAR == 1
			return std::transform_reduce(
				std::execution::par,
				a1, a2, 
				b1,
				c,
				std::plus<>(),
				std::multiplies<>()
			);
		#else
			std::atomic<T> result{0};
			async_for_each(
				a1, a2,
				[&result, &a1, &b1](const T& val, const size_t& i) 
				{
					double partial = val * b1[i];
					result.fetch_add(partial, std::memory_order_relaxed);
				},
				threads
			);
		c = result.load(std::memory_order_relaxed);
		#endif
	}

	 /** \brief A Utility for taking the outer product of vectors.
	 *
	 *	Optionally specify the type (T) of the vectors a, b, with outer_product<T>(...) 
	 *	Requires that a and b are contiguous storage.
	 *	\param a1 base address of a
	 *	\param a2 end address of a	
	 *	\param b1 base address of b
	 *	\param b2 end address of b
	 *  \prama c storage of rank 2, shape a x b 		 	 
	 */
	template <typename T = double, typename I1, typename I2>
	void 
	outer_product(I1 a1, I1 a2, I2 b1, I2 b2, T** c) 
	{
		size_t rows = std::distance(a1, a2);
		size_t cols = std::distance(b1, b2);
		const auto a = &(*a1);
		const auto b = &(*b1);

		for (size_t i = 0; i < rows; ++i)
			for (size_t j = 0; j < cols; ++j)
				c[i][j] = a[i] * b[j];
	}

	 /** \brief A Utility for calculating the outer product of a vector in parallel  
	 *
	 * 	Optionally specify the type (T) of the vector with async_outer_product<T>(...)
	 *  \param a1 base address of a
	 *	\param a2 end address of a	
	 *	\param b1 base address of b
	 *	\param b2 end address of b
	 *  \prama c storage of rank 2, shape a x b 		 	 
	 *  
	 */
	template <typename T = double, typename I1, typename I2>
	void 
	async_outer_product(I1 a1, I1 a2, I2 b1, I2 b2, T** c, 
		size_t threads = std::thread::hardware_concurrency()) 
	{
		size_t rows = std::distance(a1, a2);
		size_t cols = std::distance(b1, b2);
		const auto a = &(*a1);
		const auto b = &(*b1);

		#if PAR == 1
			 std::for_each(std::execution::par, a1, a2,
				[&a, &b, &c, &cols](const auto& ai) 
				{
					size_t i = &ai - &a[0];
					for (size_t j = 0; j < cols; ++j)
						c[i][j] = ai * (*(b + j));
				});
		#else
			async_for_each(
				a1, a2, 
				[&a, &b, &c, &cols](const auto& ai) 
				{
					size_t i = &ai - &a[0];
					for (size_t j = 0; j < cols; ++j)
						c[i][j] = ai * (*(b + j));
				},
				threads);
		#endif

	}
}
#endif //__LIB_VEC_H__