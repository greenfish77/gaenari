#ifndef HEADER_EXCEPTIONS_HPP

class base_error: public std::runtime_error {
public:
	template<typename T>
	base_error(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line) : std::runtime_error(msg), pretty_function{ pretty_function }, file{ file }, line{ line } {}
public:
	std::string pretty_function;
	std::string file;
	int line;
};

class fail: public base_error {
public:
	template<typename T>
	fail(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line) : base_error{ msg, pretty_function, file, line } {}
};

// throw macro.
#define TEST_FAIL(text)					throw fail(text,												PRETTY_FUNCTION, __FILE__, __LINE__)
#define TEST_FAIL1(text, a)				throw fail(gaenari::common::f(text, {(a)}),						PRETTY_FUNCTION, __FILE__, __LINE__)
#define TEST_FAIL2(text, a, b)			throw fail(gaenari::common::f(text, {(a), (b)}),				PRETTY_FUNCTION, __FILE__, __LINE__)
#define TEST_FAIL3(text, a, b, c)		throw fail(gaenari::common::f(text, {(a), (b), (c)}),			PRETTY_FUNCTION, __FILE__, __LINE__)
#define TEST_FAIL4(text, a, b, c, d)	throw fail(gaenari::common::f(text, {(a), (b), (c), (d)}),		PRETTY_FUNCTION, __FILE__, __LINE__)
#define TEST_FAIL5(text, a, b, c, d, e)	throw fail(gaenari::common::f(text, {(a), (b), (c), (d), (e)}),	PRETTY_FUNCTION, __FILE__, __LINE__)

#endif // HEADER_EXCEPTIONS_HPP