#ifndef HEADER_GAENARI_GAENARI_COMMON_TIME_HPP
#define HEADER_GAENARI_GAENARI_COMMON_TIME_HPP

namespace gaenari {
namespace common {

inline int64_t current_yyyymmddhhmmss(void) {
	int64_t ret = 0;
	struct tm* t;

#ifdef _WIN32
	__time64_t now = _time64(NULL);
	struct tm _t;
	_localtime64_s(&_t, &now);
	t = &_t;
#else
	// not thread-safe, but reduce the chance of an issue.
	// (copy the result to memory and use it.)
	time_t now = time(NULL);
	t = localtime(&now);
	auto __t = *t;
	t = &__t;
#endif
	ret  = (t->tm_year + 1900LL)* 10000000000LL;
	ret += (t->tm_mon + 1LL)	* 100000000LL;
	ret += (t->tm_mday)			* 1000000LL;
	ret += (t->tm_hour)			* 10000LL;
	ret += (t->tm_min)			* 100LL;
	ret += (t->tm_sec);

	return ret;
}

class elapsed_time {
public:
	elapsed_time()  {
		started = std::chrono::steady_clock::now();
	}
	~elapsed_time() = default;

public:
	inline double usec(void) const {
		return sec() * 100000;
	}

	inline double msec(void) const {
		return sec() * 1000;
	}

	inline double sec(void) const {
		std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();
		return std::chrono::duration<double>(current - started).count();
	}

	inline void reset(void) {
		started = std::chrono::steady_clock::now();
	}

	inline std::string to_string(_option_in std::optional<double> elapsed = {}) {
		std::string ret;
		double _elapsed = 0;
		if (elapsed) {
			_elapsed = elapsed.value();
		} else {
			_elapsed = sec();
		}

		auto second = _elapsed;
		int min = 0, hour = 0, day = 0;
		day = static_cast<int>(second / (60.0 * 60.0 * 24.0));
		second -= day * (60.0 * 60.0 * 24.0);
		hour = static_cast<int>(second / (60.0 * 60.0));
		second -= hour * (60.0 * 60.0);
		min = static_cast<int>(second / 60.0);
		second -= min * 60.0;

		ret += this->to_string(_elapsed) + "sec";
		if (min + hour + day > 0) {
			ret += " (";
			if (day  > 0) ret += std::to_string(day)  + "day(s) ";
			if (hour > 0) ret += std::to_string(hour) + "hour(s) ";
			if (min  > 0) ret += std::to_string(min)  + "min ";
			ret += this->to_string(second) + "sec)";
		}

		return ret;
	}

protected:
	std::string to_string(_in double v) {
		char b[64] = {0,};
		if (v >= 1.0) {
			snprintf(b, 63, "%.2f", v);
		} else {
			snprintf(b, 63, "%.4f", v);
		}
		std::string ret = b;
		gaenari::common::trim_right(ret, "0");
		gaenari::common::trim_right(ret, ".");
		return ret;
	}

protected:
	std::chrono::steady_clock::time_point started;
};

class elapsed_time_logger {
public:
	elapsed_time_logger() = delete;
	elapsed_time_logger(_in const std::string& msg): msg{msg} {}
	~elapsed_time_logger() {
		gaenari::logger::info(msg + " elapsed time: " + t.to_string());
	}
protected:
	elapsed_time t;
	std::string msg;
};

} // common
} // gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_TIME_HPP
