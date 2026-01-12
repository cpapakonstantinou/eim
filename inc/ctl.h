#ifndef __CTL_H__
#define __CTL_H__

/**
 * \brief Application control.
 * \file ctl.h
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

#include <eim.h>
#include <optional>

namespace eim
{
	struct ctl
	{
		//Waveguide Geometry
		Waveguide device; ///< Waveguide type
		double t_core; ///< Thickness of core layer
		double t_slab; ///< Thickness of slab layer
		double w_slot; ///< Width of slot
		double n_box;  ///< Refractive index of substrate
		double n_core; ///< Refractive index of core
		double n_clad; ///< Refractive index of superstrate
		double n_slot; ///< Refractive index of slot region
		//Waveguide Mode
		Mode mode = TE;///< Mode to solve
		size_t pts;    ///< Number of points for mode profile
		double extent; ///< Extent to solve for mode profile
		//Sweep-able parameters
		std::vector<double> wavelengths; ///< Wavelengths to solve
		std::vector<unsigned> mode_orders; ///< Mode orders to solve
		std::vector<double> widths;      ///< Widths of core layer to solve
		std::vector<double> gaps;        ///< Slot sizes to solve
		//Output parameters
		const char* mode_logname = NULL; ///< Mode output log name
		bool mode_log = false;           ///< Mode output flag
	};

	/**
	 * \brief Parse comma-separated list of numeric types
	 * 
	 * \tparam T The numeric type
	 * \param str The list in string format
	 * \param result The list in numeric format
	 * \param min Optional qualifier for minimum allowable value
	 * \param max Optional qualifier for maximum allowable value
	 * 
	 */
	template<typename T>
	inline size_t 
	parse_numeric(const char* str, std::vector<T>& result, 
				  std::optional<T> min = std::nullopt, 
				  std::optional<T> max = std::nullopt)
	requires 
	(
		std::same_as<T, double>   ||
		std::same_as<T, float>    ||
		std::same_as<T, unsigned> ||
		std::same_as<T, int>
	)
	{
		result.clear();
		char* end = nullptr;
		const char* current = str;
		
		while (*current)
		{
			T val;
			if constexpr (std::is_same_v<T, double>)
				val = std::strtod(current, &end);
			else if constexpr (std::is_same_v<T, float>)
				val = std::strtof(current, &end);
			else if constexpr (std::is_same_v<T, unsigned>)
				val = std::strtoul(current, &end, 10);
			else // int
				val = std::strtol(current, &end, 10);
			
			if (current == end) break;
			
			// Check bounds if specified
			if (min.has_value() && val < min.value())
				throw std::runtime_error(std::string(str) + " out of bounds (below minimum)");
			if (max.has_value() && val > max.value())
				throw std::runtime_error(std::string(str) + " out of bounds (above maximum)");
			
			result.push_back(val);
			current = (*end) ? end + 1 : end;
		}
		
		return result.size();
	}

}//namespace eim

#endif //__CTL_H__