#ifndef HEADER_GAENARI_GAENARI_LOGGER_LOGGER_HPP
#define HEADER_GAENARI_GAENARI_LOGGER_LOGGER_HPP

namespace gaenari {
namespace logger {

// public method
//
// log initialize
// - init1(...), init2(...)
//
// log with parameter.
// - info("fmt{index}", {params})
//
// log with parameter and color.
// - info("fmt{index:red}", {params})
//
// simple log.
// - info("text")
//
// log text with color tag.
// - info("hello,<red>world</red>", true)
//
// similar to info(...)
// - warn
// - err

// example)
// - info("hello, world")
// - info("hello,{0},{1},{2},{0}", {"world", "jack", 1.1})
// - info("hello,{0:red},{1},{2},{0}", {"world", "jack", 1.1})
// - info("hello,<red>world</red>", true)
//
// - support color tag:
//   . red
//   . green
//   . blue
//   . yellow
//   . magenta
//   . cyan
//   . black
//   . white
//   . light_red     (=lred)
//   . light_green   (=lgreen)
//   . light_blue    (=lblue)
//   . light_yellow  (=lyellow)
//   . light_magenta (=lmagenta)
//   . light_cyan    (=lcyan)
//   . light_gray    (=lgray)

// global flag for log initialized.
// use a convenient `volatile bool` instead of std::atomic<bool>.
static volatile bool _initialized = false;

struct tag_info {
	const char* name;
	const char* open_name;
	const char* close_name;
	const char* color;
};

// https://www.codeproject.com/Tips/5255355/How-to-Put-Color-on-Windows-Console
static const tag_info _tag_info[] = {
	{"red",				"<red>",			"</red>",			"\033[31m"},
	{"green",			"<green>",			"</green>",			"\033[32m"},
	{"blue",			"<blue>",			"</blue>",			"\033[34m"},
	{"yellow",			"<yellow>",			"</yellow>",		"\033[33m"},
	{"magenta",			"<magenta>",		"</magenta>",		"\033[35m"},
	{"cyan",			"<cyan>",			"</cyan>",			"\033[36m"},
	{"gray",			"<gray>",			"</gray>",			"\033[90m"},
	{"light_red",		"<light_red>",		"</light_red>",		"\033[91m"},
	{"light_green",		"<light_green>",	"</light_green>",	"\033[92m"},
	{"light_blue",		"<light_blue>",		"</light_blue>",	"\033[94m"},
	{"light_yellow",	"<light_yellow>",	"</light_yellow>",	"\033[93m"},
	{"light_magenta",	"<light_magenta>",	"</light_magenta>",	"\033[95m"},
	{"light_cyan",		"<light_cyan>",		"</light_cyan>",	"\033[96m"},
	{"light_gray",		"<light_gray>",		"</light_gray>",	"\033[37m"},
	{"lred",			"<lred>",			"</lred>",			"\033[91m"},
	{"lgreen",			"<lgreen>",			"</lgreen>",		"\033[92m"},
	{"lblue",			"<lblue>",			"</lblue>",			"\033[94m"},
	{"lyellow",			"<lyellow>",		"</lyellow>",		"\033[93m"},
	{"lmagenta",		"<lmagenta>",		"</lmagenta>",		"\033[95m"},
	{"lcyan",			"<lcyan>",			"</lcyan>",			"\033[96m"},
	{"lgray",			"<lgray>",			"</lgray>",			"\033[37m"},
	{"black",			"<black>",			"</black>",			"\033[30m"},
	{"white",			"<white>",			"</white>",			"\033[97m"},
	{nullptr,			nullptr,			nullptr,			nullptr},
};

struct logger_ctx {
	std::mutex mutex;
	std::string log_file_path;
	std::string text;
	std::string out;
	std::string raw;
	std::string heading_info;
	std::string heading_warn;
	std::string heading_err;
	std::string heading_info_out;
	std::string heading_warn_out;
	std::string heading_err_out;
	const std::string reset = "\033[0m";
	char current_time[32] = {0,};
	int fd = -1;
	size_t capacity = 10*1024*1024;	// 10MB log file capacity.
	bool support_color = false;
	unsigned int log_count = 0;
	std::string empty_string;
	std::map<std::string,std::string> color_map;
	std::vector<size_t> tag_open_names_len;
	std::vector<size_t> tag_close_names_len;
	std::vector<std::string> tag_names;

	logger_ctx() = default;
	~logger_ctx() {
		_initialized = false;
		std::lock_guard<std::mutex> guard(this->mutex);
		if (this->fd != -1) {
#ifdef _WIN32
			_close(this->fd);
#else
			close(this->fd);
#endif
			this->fd = -1;
		}
	}
};

// global logger instance.
static logger_ctx _instance;

// use static_assert(false, ... -> static_assert(dependent_false_v<T>, ...
template<typename> inline constexpr bool dependent_false_v = false;

// helper api class
struct _helper {
	static inline bool _is_support_console_color(void) {
#ifndef _WIN32
		// all linux distributions support.
		return true;
#else
		// windows 10 supports.
		// check windows 10.
		// WIN32 windows codes...
		DWORD major = 0;
		DWORD minor = 0;
		DWORD build = 0;

		typedef LONG(WINAPI* LPFN_RTLGETVER) (LPOSVERSIONINFOEXW);
		LPFN_RTLGETVER fnRtlGetVer;
 
		HMODULE h = ::GetModuleHandleW(L"ntdll");
		if (not h) {
			fprintf(stderr, "fail to load ntdll.");
			std::abort();
		}
		fnRtlGetVer = (LPFN_RTLGETVER)::GetProcAddress(h, "RtlGetVersion");
		if (fnRtlGetVer) {
			OSVERSIONINFOEXW osInfo = {0,};
			osInfo.dwOSVersionInfoSize = sizeof(osInfo);
			(*fnRtlGetVer)(&osInfo);
			major = osInfo.dwMajorVersion;
			minor = osInfo.dwMinorVersion;
			build = osInfo.dwBuildNumber;
		}

		// under windows 10.
		if (major < 10) return false;

		HANDLE hOut = NULL;
		DWORD  dwMode = 0;

		hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE) return false;
		if (not ::GetConsoleMode(hOut, &dwMode)) return false;
		if (not ::SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return false;

		hOut = ::GetStdHandle(STD_ERROR_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE) return false;
		if (not ::GetConsoleMode(hOut, &dwMode)) return false;
		if (not ::SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return false;
		return true;
#endif
	}

	static inline void _get_current_time(char* current_time, size_t cch) {
		char buf[32];
		buf[31] = '\0';
#ifdef _WIN32
		time_t		tm_time = 0;
		struct tm	st_time = {0,};
		time(&tm_time);
		localtime_s(&st_time, &tm_time);
		strftime(current_time, cch-1, "[%Y/%m/%d %H:%M:%S]", &st_time);
#else
		time_t		tm_time	= 0;
		struct tm*	st_time	= nullptr;
		time(&tm_time);
		st_time = localtime(&tm_time);
		strftime(current_time, cch-1, "[%Y/%m/%d %H:%M:%S]", st_time);
#endif
	}

	static inline void _write_to_file_under_locked(const std::string& raw) {
		size_t filesize = 0;
		if (_instance.fd == -1) return;

		// for performance, file size check every 128 log count.
		if (0 == ((_instance.log_count++) % 128)) {
			struct stat buf;
			if (-1 != fstat(_instance.fd, &buf)) filesize = buf.st_size;
		}

		// file size is big.
		if (filesize > _instance.capacity) {
			// close file
#ifdef _WIN32
			_close(_instance.fd);
#else
			close(_instance.fd);
#endif
			_instance.fd = -1;
			// remove old.
			::remove((_instance.log_file_path + ".old.log").c_str());
			// rename.
			if (0 != ::rename(_instance.log_file_path.c_str(), (_instance.log_file_path + ".old.log").c_str())) {/* what shuld i do?*/}
			
			// open new
#ifdef _WIN32
			_sopen_s(&_instance.fd, _instance.log_file_path.c_str(), O_TRUNC | O_RDWR | O_CREAT | O_APPEND | O_BINARY, SH_DENYNO, S_IREAD | S_IWRITE);
#else
			_instance.fd = open(_instance.log_file_path.c_str(), O_TRUNC | O_RDWR | O_CREAT | O_APPEND | 0, 0x444 | S_IREAD | S_IWRITE);
#endif
		}

#ifdef _WIN32
		_write(_instance.fd, raw.c_str(), static_cast<unsigned int>(raw.length()));
#else
		write(_instance.fd, raw.c_str(), static_cast<unsigned int>(raw.length()));
#endif
	}

	static inline void _init_tag_info(void) {
		if (_instance.tag_names.empty()) {
			for (size_t i = 0;;i++) {
				if (not _tag_info[i].name) break;
				_instance.tag_open_names_len.push_back(strlen(_tag_info[i].open_name));
				_instance.tag_close_names_len.push_back(strlen(_tag_info[i].close_name));

				// std::string may operate faster than const char*.
				_instance.tag_names.push_back(_tag_info[i].name);
				_instance.color_map[_tag_info[i].name] = _tag_info[i].color;
			}
		}
	}

	static inline void _set_heading(void) {
		_instance.heading_info = "[info]";
		_instance.heading_warn = "[warn]";
		_instance.heading_err  = "[err ]";
		if (_instance.support_color) {
			_instance.heading_info_out = "\033[92m[info]\033[0m";
			_instance.heading_warn_out = "\033[33m[warn]\033[0m";
			_instance.heading_err_out  = "\033[91m[err ]\033[0m";
		} else {
			_instance.heading_info_out = "[info]";
			_instance.heading_warn_out = "[warn]";
			_instance.heading_err_out  = "[err ]";
		}
	}

	static inline void _shrink(void) {
		if (_instance.text.capacity() > 65536) {
			_instance.text.clear();
			_instance.text.shrink_to_fit();
		}
		if (_instance.out.capacity() > 65536) {
			_instance.out.clear();
			_instance.out.shrink_to_fit();
		}
		if (_instance.raw.capacity() > 65536) {
			_instance.raw.clear();
			_instance.raw.shrink_to_fit();
		}
	}

	static inline std::string _to_string_double(const double v) {
		std::string f = std::to_string(v);
		f.erase(f.find_last_not_of('0') + 1, std::string::npos);
		if (f.back() == '.') f.pop_back();
		return f;
	}

	static inline bool _find_tag(const char* s, size_t& index) {
		for (size_t i=0;;i++) {
			if (not _tag_info[i].name) break;
			if (0 == strncmp(s, _tag_info[i].open_name, _instance.tag_open_names_len[i])) {
				// found.
				index = i;
				return true;
			}
		}
		return false;
	}

	// "hello, <red>world</red>" -> "hello, {0:red}", {"world"}
	// warning) code is not optimized.
	static inline void _color_tag_split(const std::string& s, std::string& fmt, std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>& params) {
		const char* cur = &s[0];
		bool in_tag = false;
		size_t find_tag_index = 0;
		std::string tag_name;
		std::string cur_tag_name;
		std::string inner_text;
		fmt.reserve(s.length());
		fmt.clear();
		params.clear();
		for (;;cur++) {
			if (not *cur) break;
			if ('<' == *cur) {
				if (_helper::_find_tag(cur, find_tag_index)) {
					// found tag. (<red>)
					// find close tag. (</red>)
					const char* close_tag = strstr(cur, _tag_info[find_tag_index].close_name);
					if (close_tag) {
						// found close tag. (</red>)
						std::string inner_text = std::string(cur + _instance.tag_open_names_len[find_tag_index], close_tag - cur - _instance.tag_open_names_len[find_tag_index]);
						fmt.push_back('{');
						fmt += std::to_string(params.size());
						fmt.push_back(':');
						fmt += _instance.tag_names[find_tag_index];
						fmt.push_back('}');
						params.emplace_back(inner_text);
						cur = close_tag + _instance.tag_close_names_len[find_tag_index] - 1;
						continue;
					}
				}
			} else if ('{' == *cur) {
				// { -> {{
				fmt.push_back(*cur);
			} else if ('}' == *cur) {
				// } -> }}
				fmt.push_back(*cur);
			}
			fmt.push_back(*cur);
		}
	}

	// ex) 
	// fmt    : "hello,{0},{1}."
	// params : {"world","Jack"}
	// text   : "hello,world,Jack."
	// out    : "hello,world,Jack."
	//
	// fmt    : "hello,{0:red},{1}."
	// params : {"world","Jack"}
	// text   : "hello,world,Jack."
	// out    : "hello,\x1b[31mworld\x1b[0m,Jack."
	static inline void _format_msg(const std::string& fmt, const std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>& params, std::string& text, std::string& out) {
		size_t i = 0;
		bool malformed = false;
		bool in_block = false;
		bool in_double_bracket = false;
		const char* c = &fmt[0];
		size_t l = fmt.length();
		std::string block;
		std::string index;

		text.clear();
		out.clear();
		text.reserve(fmt.length() + 256);
		out.reserve(fmt.length() + 256);

		for (i=0; i<l; i++) {
			if (not in_block) {
				if (c[i] != '{') {
					text.push_back(c[i]);
					out.push_back(c[i]);
					continue;
				}
			}

			if (c[i] == '}') {
				if (in_double_bracket) {
					if (c[i+1] != '}') {
						malformed = true;
						continue;
					}

					in_block = false;
					in_double_bracket = false;
					continue;
				}
			}

			if (in_double_bracket) {
				text.push_back(c[i]);
				out.push_back(c[i]);
				continue;
			}

			if (not in_block) {
				// found '{'.
				if (c[i+1] == '{') {
					// found '{{'
					in_double_bracket = true;
				}

				in_block = true;
				block.clear();
				continue;
			}

			if (c[i] == '}') {
				in_block = false;
				if (in_double_bracket) {
					if (c[i+1] != '}') malformed = true;
					in_double_bracket = false;
					continue;
				}

				// tag completed.
				const char* tags = nullptr;
				size_t hash_sign = block.find(':');
				size_t param_index = 0;
				if (hash_sign != std::string::npos) { 
					tags = &block[hash_sign+1];
					index = block.substr(0, hash_sign);
				} else {
					index = block;
				}
			
				if (not (index.find_first_not_of("0123456789") == std::string::npos)) {
					malformed = true;
					continue;
				}

				try {
					param_index = static_cast<size_t>(std::stoll(index));
				} catch (...) {
					malformed = true;
					continue;
				}

				if (param_index >= params.size()) {
					malformed = true;
					continue;
				}

				// tag processed.
				// - param_index : {n}   n value.
				// - color       : {n:c} c value. (no color is nullptr.)
				if (tags) {

					auto find = _instance.color_map.find(tags);
					if (find == _instance.color_map.end()) {
						malformed = true;
						continue;
					}

					// add color text(\033[31m, ...)
					out += find->second;
				}

				auto variant_index = params[param_index].index();
				std::string value;
				try {
					if      (1 == variant_index) value = std::to_string(std::get<1>(params[param_index]));		// int
					else if (2 == variant_index) value = std::to_string(std::get<2>(params[param_index]));		// int64_t
					else if (3 == variant_index) value = _to_string_double(std::get<3>(params[param_index]));	// <double> 9.900000 -> 9.9
					else if (4 == variant_index) value = std::get<4>(params[param_index]);						// string
					else if (5 == variant_index) value = std::to_string(std::get<5>(params[param_index]));		// size_t
				} catch (...) {
				}

				text += value;
				out  += value;

				// add reset color text.
				if (tags) out += _instance.reset;
			}

			block.push_back(c[i]);
		}
	}
};

// log with file and console.
// can be recalled.
inline void init1(const std::string& log_file_path, bool color=true, size_t capacity=10*1024*1024) {
	std::lock_guard<std::mutex> guard(_instance.mutex);
	_instance.capacity = capacity;
	_instance.log_file_path = log_file_path;
#ifdef _WIN32
	if (-1 != _instance.fd) _close(_instance.fd);
	_instance.fd = -1;
	_sopen_s(&_instance.fd, log_file_path.c_str(), O_RDWR | O_CREAT | O_APPEND | O_BINARY, SH_DENYNO, S_IREAD | S_IWRITE);
	if ((not _instance.support_color) and color) _instance.support_color = _helper::_is_support_console_color();
#else
	_instance.fd = open(log_file_path.c_str(), O_RDWR | O_CREAT | O_APPEND | 0, 0x444 | S_IREAD | S_IWRITE);
	_instance.support_color = color;
#endif
	_helper::_set_heading();
	_helper::_init_tag_info();
	_initialized = true;
}

// log with only console.
// can be recalled.
inline void init2(bool color=true) {
	std::lock_guard<std::mutex> guard(_instance.mutex);
#ifdef _WIN32
	if (_instance.fd != -1) _close(_instance.fd);
	if ((not _instance.support_color) and color) _instance.support_color = _helper::_is_support_console_color();
#else
	if (_instance.fd != -1) close(_instance.fd);
	_instance.support_color = color;
#endif
	_instance.fd = -1;
	_helper::_set_heading();
	_helper::_init_tag_info();
	_initialized = true;
}

inline const tag_info* get_tag_info(void) {
	return _tag_info;
}

inline void info(const std::string& fmt, const std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>& params) {
	if (not _initialized) return;
	std::lock_guard<std::mutex> guard(_instance.mutex);
	if (not _initialized) return;
	_helper::_get_current_time(_instance.current_time, sizeof(_instance.current_time));
	_helper::_format_msg(fmt, params, _instance.text, _instance.out);
	fprintf(stdout, "%s%s %s\n", _instance.current_time, _instance.heading_info_out.c_str(), _instance.support_color ? _instance.out.c_str() : _instance.text.c_str());
	_instance.raw  = _instance.current_time;
	_instance.raw += _instance.heading_info;
	_instance.raw += ' ';
	_instance.raw += _instance.text;
	_instance.raw += '\n';
	_helper::_write_to_file_under_locked(_instance.raw);
	_helper::_shrink();
}

template <typename T>
inline void info(const std::string& text, const T& color_tagged) {
	if (not _initialized) return;
	if constexpr(std::is_same<T, bool>::value) {
		if (not color_tagged) { 
			info(text, std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>{});
		} else {
			std::string fmt;
			std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>> params;
			_helper::_color_tag_split(text, fmt, params);
			info(fmt, params);
		}
	} else {
		static_assert(dependent_false_v<T>, "invalid second parameter type. bool or vector<variant> allowed.");
	}
}

inline void info(const std::string& text) {
	info(text, false);
}

inline void warn(const std::string& fmt, const std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>& params) {
	if (not _initialized) return;
	std::lock_guard<std::mutex> guard(_instance.mutex);
	if (not _initialized) return;
	_helper::_get_current_time(_instance.current_time, sizeof(_instance.current_time));
	_helper::_format_msg(fmt, params, _instance.text, _instance.out);
	fprintf(stdout, "%s%s %s\n", _instance.current_time, _instance.heading_warn_out.c_str(), _instance.support_color ? _instance.out.c_str() : _instance.text.c_str());
	_instance.raw  = _instance.current_time;
	_instance.raw += _instance.heading_warn;
	_instance.raw += ' ';
	_instance.raw += _instance.text;
	_instance.raw += '\n';
	_helper::_write_to_file_under_locked(_instance.raw);
	_helper::_shrink();
}
template <typename T>
inline void warn(const std::string& text, const T& color_tagged) {
	if (not _initialized) return;
	if constexpr(std::is_same<T, bool>::value) {
		if (not color_tagged) {
			warn(text, std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>{});
		} else {
			std::string fmt;
			std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>> params;
			_helper::_color_tag_split(text, fmt, params);
			warn(fmt, params);
		}
	} else {
		static_assert(dependent_false_v<T>, "invalid second parameter type. bool or vector<variant> allowed.");
	}
}

inline void warn(const std::string& text) {
	warn(text, false);
}

inline void error(const std::string& fmt, const std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>& params) {
	if (not _initialized) {
		fprintf(stderr, "%s\n", fmt.c_str());
		return;
	}
	std::lock_guard<std::mutex> guard(_instance.mutex);
	if (not _initialized) return;
	_helper::_get_current_time(_instance.current_time, sizeof(_instance.current_time));
	_helper::_format_msg(fmt, params, _instance.text, _instance.out);
	fprintf(stderr, "%s%s %s\n", _instance.current_time, _instance.heading_err_out.c_str(), _instance.support_color ? _instance.out.c_str() : _instance.text.c_str());
	_instance.raw  = _instance.current_time;
	_instance.raw += _instance.heading_err;
	_instance.raw += ' ';
	_instance.raw += _instance.text;
	_instance.raw += '\n';
	_helper::_write_to_file_under_locked(_instance.raw);
	_helper::_shrink();
}

template <typename T>
inline void error(const std::string& text, const T& color_tagged) {
	if (not _initialized) {
		fprintf(stderr, "%s\n", text.c_str());
		return;
	}
	if constexpr(std::is_same<T, bool>::value) {
		if (not color_tagged) { 
			error(text, std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>>{});
		} else {
			std::string fmt;
			std::vector<std::variant<std::monostate,int,int64_t,double,std::string,size_t>> params;
			_helper::_color_tag_split(text, fmt, params);
			error(fmt, params);
		}
	} else {
		static_assert(dependent_false_v<T>, "invalid second parameter type. bool or vector<variant> allowed.");
	}
}

inline void error(const std::string& text) {
	error(text, false);
}

} // namespace logger
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_LOGGER_LOGGER_HPP
