#ifndef HEADER_EXCEPTIONS_HPP
#define HEADER_EXCEPTIONS_HPP

class base_error : public std::runtime_error {
public:
	template<typename T>
	base_error(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line) : std::runtime_error(msg), pretty_function{ pretty_function }, file{ file }, line{ line } {}
public:
	std::string pretty_function;
	std::string file;
	int line;
};

class error: public base_error {
public:
	template<typename T>
	error(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line) : base_error{ msg, pretty_function, file, line } {}
};

// throw macro.
#define ERROR0(text)				throw error(text,												PRETTY_FUNCTION, __FILE__, __LINE__)
#define ERROR1(text, a)				throw error(gaenari::common::f(text, {(a)}),						PRETTY_FUNCTION, __FILE__, __LINE__)
#define ERROR2(text, a, b)			throw error(gaenari::common::f(text, {(a), (b)}),				PRETTY_FUNCTION, __FILE__, __LINE__)
#define ERROR3(text, a, b, c)		throw error(gaenari::common::f(text, {(a), (b), (c)}),			PRETTY_FUNCTION, __FILE__, __LINE__)
#define ERROR4(text, a, b, c, d)	throw error(gaenari::common::f(text, {(a), (b), (c), (d)}),		PRETTY_FUNCTION, __FILE__, __LINE__)
#define ERROR5(text, a, b, c, d, e)	throw error(gaenari::common::f(text, {(a), (b), (c), (d), (e)}),	PRETTY_FUNCTION, __FILE__, __LINE__)

#endif // HEADER_EXCEPTIONS_HPP
