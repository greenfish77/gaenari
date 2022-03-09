#ifndef HEADER_GAENARI_SUPUL_EXCEPTIONS_EXCEPTIONS_HPP
#define HEADER_GAENARI_SUPUL_EXCEPTIONS_EXCEPTIONS_HPP

namespace supul {
namespace exceptions {

class base_error: public std::runtime_error {
public:
	template<typename T>
	base_error(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line): std::runtime_error(msg), pretty_function{pretty_function}, file{file}, line{line} {}
public:
	std::string pretty_function;
	std::string file;
	int line;
};

class error: public base_error {
public:
	template<typename T>
	error(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line): base_error{msg, pretty_function, file, line} {}
};

class internal_error : public base_error {
public:
	template<typename T>
	internal_error(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line) : base_error{ msg, pretty_function, file, line } {}
};

class db_error : public base_error {
public:
	template<typename T>
	db_error(_in const T& msg, _in const std::string& desc, _in const char* pretty_function, _in const char* file, _in int line) : base_error{msg, pretty_function, file, line}, desc{desc} {}
public:
	std::string desc;
};

class not_matched_error : public base_error {
public:
	template<typename T>
	not_matched_error(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line): base_error{msg, pretty_function, file, line} {}
};

class not_supported_yet : public base_error {
public:
	template<typename T>
	not_supported_yet(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line): base_error{msg, pretty_function, file, line} {}
};

class item_not_found : public base_error {
public:
	template<typename T>
	item_not_found(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line): base_error{msg, pretty_function, file, line} {}
};

class invalid_data_type : public base_error {
public:
	template<typename T>
	invalid_data_type(_in const T& msg, _in const char* pretty_function, _in const char* file, _in int line): base_error{msg, pretty_function, file, line} {}
};

// throw macro.
#define THROW_SUPUL_ERROR(text)						throw exceptions::error(text, PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_INTERNAL_ERROR0					throw exceptions::internal_error("internal error", PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_INTERNAL_ERROR1(text)			throw exceptions::internal_error(text, PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_DB_ERROR(text, desc)			throw exceptions::db_error(text, desc, PRETTY_FUNCTION, __FILE__, __LINE__);
#define THROW_SUPUL_RULE_NOT_MATCHED_ERROR(text)	throw exceptions::not_matched_error(text, PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_NOT_SUPPORTED_YET(text)			throw exceptions::not_supported_yet(text, PRETTY_FUNCTION, __FILE__, __LINE__)

// tweak.
#define THROW_SUPUL_ERROR1(text,a)					throw exceptions::error(F(text, {a}),			PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_ERROR2(text,a,b)				throw exceptions::error(F(text, {a,b}),			PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_ERROR3(text,a,b,c)				throw exceptions::error(F(text, {a,b,c}),		PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_ERROR4(text,a,b,c,d)			throw exceptions::error(F(text, {a,b,c,d}),		PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_ERROR5(text,a,b,c,d,e)			throw exceptions::error(F(text, {a,b,c,d,e}),	PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_ITEM_NOT_FOUND(item_name)		throw exceptions::item_not_found(F("item not found: %0.", {item_name}), PRETTY_FUNCTION, __FILE__, __LINE__)
#define THROW_SUPUL_INVALID_DATA_TYPE(item_name)	throw exceptions::invalid_data_type(F("invalid data type: %0.", {item_name}), PRETTY_FUNCTION, __FILE__, __LINE__)

// centeralized exception handling.
// catch all, and return reason text.
// https://stackoverflow.com/questions/48036435/reusing-exception-handling-code-in-c
// http://cppsecrets.blogspot.com/2013/12/using-lippincott-function-for.html
inline std::string catch_all() noexcept {
	try {
		throw;
	} catch (const exceptions::error& e) {
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>error</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("       <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("       <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w;
	} catch (const exceptions::not_matched_error& e) {
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>not matched error</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("                   <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("                   <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w;
	} catch (const exceptions::internal_error& e) {
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>internal error</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("                <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("                <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w;
	} catch (const exceptions::db_error& e) {
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>database error</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("                <yellow>" + e.desc + "</yellow>", true);
		gaenari::logger::error("                <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("                <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w;
	} catch (const exceptions::not_supported_yet& e) {
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>not supported yet</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("                   <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("                   <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w;
	} catch (const exceptions::item_not_found& e) {
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>item not found</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("                <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("                <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w;
	} catch (const exceptions::invalid_data_type& e) {
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>invalid data type</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("                   <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("                   <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w;
	} catch (const gaenari::exceptions::base_error& e) {
		std::string w = e.what();
		gaenari::logger::error("<red>gaenari error</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("         code: <yellow>" + e.code + "</yellow>", true);
		gaenari::logger::error("               <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("               <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
		return w + "(" + e.code + ")";
	} catch (const gaenari::common::json_insert_order_map::base& e) {
		std::string w = e.what();
		gaenari::logger::error("<red>json error</red>: <yellow>" + w + "</yellow>", true);
		return w;
	} catch (const std::exception& e) {
		std::string w = e.what();
		gaenari::logger::error("<red>std::exception</red>: <yellow>" + w + "</yellow>", true);
		return w;
	} catch (...) {
		std::string w = "N/A";
		gaenari::logger::error("<red>undefined error.</red>", true);
		return w;
	}
}

} // exceptions
} // supul

#endif // HEADER_GAENARI_SUPUL_EXCEPTIONS_EXCEPTIONS_HPP
