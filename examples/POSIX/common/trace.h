#ifndef _TRACE_H
#define _TRACE_H
#include <iostream>
#include <ios>
#include <cstdint>
#include "buffer.h"

namespace posix
{

//#define TRACE_CODE
//#undef TRACE_CODE

template <typename T>
void trace(std::ostream &stream, T &t)
{
	stream << t;
}

template <typename T, typename... Args>
void trace(std::ostream &stream, T &t, Args... args)
{
	stream << t;
	trace(stream, args...);
}

inline void trace_array(std::ostream &stream, Buffer &a)
{
	stream << "{ ";
	for (std::size_t i = 0; i < a.offset()-1; ++i)
	{
		stream << std::hex << (int)a.data()[i] << ", ";
		if (i && ((i + 1) % 16 == 0))
			stream << "\n";
	}
	stream << std::hex << (int)a.data()[a.offset()-1] << std::dec  << " }\n";
}

template <typename T>
void trace_array(std::ostream &stream, std::vector<T> &a)
{
	stream << "{ ";
	for (std::size_t i = 0; i < a.size() - 1; ++i)
	{
		stream << a[i] << ", ";
	}
	stream << a[a.size()-1] << " }\n";
}

template <typename T, std::size_t N>
void trace_array(std::ostream &stream, std::array<T, N> &a)
{
	stream << "{ ";
	for (std::size_t i = 0; i < N-1; ++i)
	{
		stream << a[i] << ", ";
	}
	stream << a[N-1] << " }\n";
}

inline void trace_array(std::ostream &stream, std::vector<uint8_t> &a)
{
	stream << "{ ";
	for (std::size_t i = 0; i < a.size() - 1; ++i)
	{
		stream << std::hex << (int)a[i] << ", ";
		if (i && ((i + 1) % 16 == 0))
			stream << "\n";
	}
	stream << std::hex << (int)a[a.size()-1] << std::dec << " }\n";
}

template <typename std::size_t N>
void trace_array(std::ostream &stream, std::array<uint8_t, N> &a)
{
	stream << "{ ";
	for (std::size_t i = 0; i < N-1; ++i)
	{
		stream << std::hex << (int)a[i] << ", ";
		if (i && ((i + 1) % 16 == 0))
			stream << "\n";
	}
	stream << std::hex << (int)a[N-1] << std::dec << " }\n";
}

template <typename std::size_t N>
void trace_array(std::ostream &stream, const uint8_t (&a)[N])
{
	stream << "{ ";
	for (std::size_t i = 0; i < N-1; ++i)
	{
		stream << std::hex << (int)a[i] << ", ";
		if (i && ((i + 1) % 16 == 0))
			stream << "\n";
	}
	stream << std::hex << (int)a[N-1] << std::dec << " }\n";
}

template <typename T, std::size_t N>
void trace_array(std::ostream &stream, const T(&a)[N])
{
	stream << "{ ";
	for (std::size_t i = 0; i < N-1; ++i)
	{
		stream << a[i] << ", ";
	}
	stream << a[N-1] << " }\n";
}

#ifdef TRACE_CODE
#define TRACE(...) trace(std::cout, "==TRACING== (", __func__, ":", __LINE__, ") | ", __VA_ARGS__)
#define TRACE_ARRAY(array) do{\
	TRACE("");\
	trace_array(std::cout, array);\
}while(0)
#else
#define TRACE(...)
#define TRACE_ARRAY(array)
#endif

#define ENTER_TRACE() TRACE("<<< Entering\n")
#define EXIT_TRACE() TRACE(">>> Exiting\n")

}

#endif
