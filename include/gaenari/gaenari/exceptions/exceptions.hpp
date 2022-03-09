#ifndef HEADER_GAENARI_GAENARI_EXCEPTIONS_HPP
#define HEADER_GAENARI_GAENARI_EXCEPTIONS_HPP

namespace gaenari {
namespace exceptions {

class base_error: public std::runtime_error {
public:
	base_error() = delete;
	base_error(const std::string& msg, _in const char* pretty_function, _in const char* file, _in int line): std::runtime_error{msg}, code{"base_error"}, pretty_function{pretty_function}, file{file}, line{line} {}
	base_error(const std::string& msg, const std::string& code, _in const char* pretty_function, _in const char* file, _in int line): std::runtime_error{msg}, code{code}, pretty_function{pretty_function}, file{file}, line{line} {}
public:
	std::string code;
	std::string pretty_function;
	std::string file;
	int line;
};

class error: public base_error {
public:
	error(const std::string& msg, const std::string& code, _in const char* pretty_function, _in const char* file, _in int line): base_error{msg, code, pretty_function, file, line} {}
};

// similar to an event, what to catch internally is defined separately.
class error_feature_not_found: public base_error {
public:
	error_feature_not_found(const std::string& msg, const std::string& code, _in const char* pretty_function, _in const char* file, _in int line): base_error{msg, code, pretty_function, file, line} {}
};

// throw macro.
#define THROW_GAENARI_ERROR(msg)				throw exceptions::error(msg, "error", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_GAENARI_FEATURE_NOT_FOUND(msg)	throw exceptions::error_feature_not_found(msg, "feature_not_found", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_GAENARI_INVALID_PARAMETER(msg)	throw exceptions::error(msg, "invalid parameter", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_GAENARI_INTERNAL_ERROR0			throw exceptions::error("",  "internal error", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_GAENARI_INTERNAL_ERROR1(msg)		throw exceptions::error(msg, "internal error", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_GAENARI_NOT_SUPPORTED_YET(msg)	throw exceptions::error(msg, "not supported yet", PRETTY_FUNCTION, __FILE__, __LINE__)

// tweak.
#define THROW_GAENARI_ERROR1(text,a)			throw exceptions::error(F(text, {a}),     "error", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_GAENARI_ERROR2(text,a,b)			throw exceptions::error(F(text, {a,b}),   "error", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_GAENARI_ERROR3(text,a,b,c)		throw exceptions::error(F(text, {a,b,c}), "error", PRETTY_FUNCTION, __FILE__, __LINE__)

} // namespace exceptions
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_EXCEPTIONS_HPP
