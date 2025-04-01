#ifndef __LOG_H__
#define __LOG_H__
/** 
 * \file 	log.h
* \brief 	definition for log
* \author 	c. papakonstantinou
* \date 	March 2023 
**/
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

#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

class Log
{
	bool newline_{true};
	std::string delim_{};
	std::ofstream log_{};
	
	public:
	explicit Log(std::string log_file, std::string delim=" "):
	log_(log_file),
	delim_(delim)
	{ }

	~Log() 
	{
		if (log_.is_open())
			log_.close();
	}
	
	void newline()
	{
		log_ << std::endl;
		newline_ = true;
	}

	template<typename T>
	friend Log& operator<<(Log& olog, T&& x)
	{
		if( olog.newline_ ) 
			olog.newline_ = false;
		else olog.log_ << olog.delim_;

		olog.log_ << std::fixed  << x;

		return olog;
	}

	void operator++()
	{
		this->newline();
	}
};

#endif //__LOG_H__