#ifndef HEADER_GAENARI_SUPUL_COMMON_FUNCTION_LOGGER_HPP
#define HEADER_GAENARI_SUPUL_COMMON_FUNCTION_LOGGER_HPP

namespace supul {
namespace common {

class function_logger {
public:
	enum class show_option {
		full				= 0,
		full_without_line	= 1,
		complete_only		= 2,
		error_only			= 3,
	};
public:
	function_logger() = delete;
	function_logger(_in const char* _func_, _option_in const char* _classname_ = nullptr, _option_in show_option option = show_option::full) {
		if (not _func_) return;
		if (not _classname_) func = std::string("api.") + _func_ + "()";
		else func = std::string("api.") + _classname_ + '.' + _func_ + "()";
		std::string msg  = "<yellow>" + func + " started.</yellow>";
		std::string line = "<yellow>" + std::string(msg.length() - 17, '-') + "</yellow>";
		_show_option = option;
		switch (option) {
			case show_option::full:
				gaenari::logger::info(line, true);
				gaenari::logger::info(msg,  true);
				gaenari::logger::info(line, true);
				break;
			case show_option::full_without_line:
				gaenari::logger::info(msg,  true);
				break;
		}
	}
	void failed(void) {
		_failed = true;
	}
	~function_logger(void) {
		std::string msg, line;
		if (_failed) {
			msg  = "<red>" + func + " failed.</red> (" + t.to_string() + ')';
			line = "<red>" + std::string(msg.length() - 11, '-') + "</red>";
		} else {
			msg  = "<yellow>" + func + " completed.</yellow> (" + t.to_string() + ')';
			line = "<yellow>" + std::string(msg.length() - 17, '-') + "</yellow>";
		}

		switch (_show_option) {
			case show_option::full:
				gaenari::logger::info(line, true);
				gaenari::logger::info(msg,  true);
				gaenari::logger::info(line, true);
				break;
			case show_option::full_without_line:
			case show_option::complete_only:
				gaenari::logger::info(msg,  true);
				break;
			case show_option::error_only:
				if (_failed) gaenari::logger::info(msg, true);
				break;
		}
	}
protected:
	gaenari::common::elapsed_time t;
	std::string func;
	bool _failed = false;
	show_option _show_option = show_option::full;
};

} // common
} // supul

#endif // HEADER_GAENARI_SUPUL_COMMON_FUNCTION_LOGGER_HPP
