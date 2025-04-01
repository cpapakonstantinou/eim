#ifndef __ASYNC_H__
#define __ASYNC_H__
/**
 * \file async.h header only support for task based parallelization using c++ async
 * \author cpapakonstantinou
 * \date 2025
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

#include <future>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <exception>

/* 	\brief	launch a true asychronous thread with parameter forwarding
 * 
 *	\param 	f 	The function to launch asynchronously
 *	\param 	params	the function arguments
 *
 *	\returns a future object for the asynchronous thread	
 */
template<typename F, typename... Ts> 
inline std::future<typename std::result_of<F(Ts...)>::type>
call_async(F&& f, Ts&&... params) 
{
	return std::async(std::launch::async, 
		std::forward<F>(f), 
		std::forward<Ts>(params)...);
}

/* 	\brief	launch a true asychronous thread with parameter forwarding
 * 
 *	\param 	f 	The function to launch asynchronously
 *	\param 	params	the function arguments
 *
 *	\returns a future object for the asynchronous thread	
 */
template<typename F, typename P = std::function<void(size_t)>, typename I>
void 
async_for_each(I begin, I end, F&& f, 
					size_t threads = std::thread::hardware_concurrency(), 
					P progress = nullptr) 
{
	auto size = std::distance(begin, end);
	if (size == 0) return;

	threads = std::min(threads, static_cast<size_t>(size));
	auto chunk_size = size / threads;

	std::vector<std::future<void>> futures(threads);
	std::mutex ex_mutex;
	std::exception_ptr ex_ptr = nullptr;
	std::atomic<bool> abort(false);
	std::atomic<unsigned> completed(0);

	auto chunk_begin = begin;
	for (size_t i = 0; i < threads; ++i) 
	{
		auto chunk_end = (i == threads - 1) ? end : std::next(chunk_begin, chunk_size);
		futures[i] = call_async( 
			[chunk_begin, chunk_end, chunk_size, &f, &ex_mutex, &ex_ptr, &abort, &completed, &progress, i]() 
			{
				try 
				{
					size_t idx = i * (chunk_end - chunk_begin);
					for (auto it = chunk_begin; it != chunk_end && !abort.load(); ++it) 
					{
						if constexpr (std::is_invocable_v<F, decltype(*it), size_t>) 
							std::invoke(std::forward<F>(f), *it, idx);
						else 
							std::invoke(std::forward<F>(f), *it);
					}

					++completed;
					
					if (progress)
						progress(completed);
				} 
				catch (...) 
				{
					std::lock_guard<std::mutex> lock(ex_mutex);
					if (!ex_ptr) 
					{
						ex_ptr = std::current_exception();
						abort.store(true);
					}
				}
			});
		chunk_begin = chunk_end;
	}

	for (auto& fut : futures) 
	{
		try 
		{
			fut.get(); // Wait for all tasks
		} 
		catch (...) 
		{
			std::lock_guard<std::mutex> lock(ex_mutex);
			if (!ex_ptr) 
			{
				ex_ptr = std::current_exception();
				abort.store(true);
			}
		}
	}

	// After all threads have joined, then propagate the error. 
	if (ex_ptr) {
		std::rethrow_exception(ex_ptr);
	}
}



#endif