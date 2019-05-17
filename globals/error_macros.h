#ifndef ERROR_MACROS_H
#define ERROR_MACROS_H

#include <signal.h>
#include <iostream>

#if 0
#ifdef WINDOWS_ENABLED

#define RAISE_SIGNAL

#endif

#ifdef POSIX_ENABLED

#define RAISE_SIGNAL \
	if (getenv("ABORT_ON_ERROR")) abort();

#endif
#endif

#define RAISE_SIGNAL

void _error_break_function();

#define ERR_FAIL_INDEX(m_index, m_size)                                                         \
	{                                                                                           \
		if ((m_index) < 0 || (m_index) >= (m_size)) {                                           \
			std::cout << " *** ERROR *** " << __FILE__ << ":" << __LINE__ << " - "              \
					  << "index out of size: " << m_index << "(" << m_size << ")" << std::endl; \
			_error_break_function();                                                            \
			RAISE_SIGNAL return;                                                                \
		}                                                                                       \
	}

#define ERR_FAIL_INDEX_V(m_index, m_size, m_retval)                                             \
	{                                                                                           \
		if ((m_index) < 0 || (m_index) >= (m_size)) {                                           \
			std::cout << " *** ERROR *** " << __FILE__ << ":" << __LINE__ << " - "              \
					  << "index out of size: " << m_index << "(" << m_size << ")" << std::endl; \
			_error_break_function();                                                            \
			RAISE_SIGNAL return (m_retval);                                                     \
		}                                                                                       \
	}

#define ERR_FAIL_COND(m_cond)                                                                                          \
	{                                                                                                                  \
		if (m_cond) {                                                                                                  \
			std::cout << " *** ERROR *** " << __FILE__ << ":" << __LINE__ << " - " << #m_cond " failed." << std::endl; \
			_error_break_function();                                                                                   \
			RAISE_SIGNAL return;                                                                                       \
		}                                                                                                              \
	}

#define ERR_FAIL_COND_V(m_cond, m_retval)                                                                              \
	{                                                                                                                  \
		if (m_cond) {                                                                                                  \
			std::cout << " *** ERROR *** " << __FILE__ << ":" << __LINE__ << " - " << #m_cond " failed." << std::endl; \
			_error_break_function();                                                                                   \
			RAISE_SIGNAL return m_retval;                                                                              \
		}                                                                                                              \
	}

#define ERR_CONTINUE(m_cond)                                                                                           \
	{                                                                                                                  \
		if (m_cond) {                                                                                                  \
			std::cout << " *** ERROR *** " << __FILE__ << ":" << __LINE__ << " - " << #m_cond " failed." << std::endl; \
			_error_break_function();                                                                                   \
			RAISE_SIGNAL continue;                                                                                     \
		}                                                                                                              \
	}

#define ERR_PRINT(m_string)                                                                              \
	{                                                                                                    \
		std::cout << " *** ERROR *** " << __FILE__ << ":" << __LINE__ << " - " << m_string << std::endl; \
		_error_break_function();                                                                         \
		RAISE_SIGNAL                                                                                     \
	}

#define WARN_PRINT(m_string)                                                                             \
	{                                                                                                    \
		std::cout << " *** ERROR *** " << __FILE__ << ":" << __LINE__ << " - " << m_string << std::endl; \
		RAISE_SIGNAL                                                                                     \
	}

#endif
