#ifndef HEADER_GAENARI_SUPUL_COMMON_MISC_HPP
#define HEADER_GAENARI_SUPUL_COMMON_MISC_HPP

namespace supul {
namespace common {

inline std::string& path_join(_in _out std::string& ret, _in const std::string& add) {
	std::filesystem::path r = ret;
	r = std::filesystem::absolute(r.append(add));
	ret = r.string();
	return ret;
}

inline std::string path_join_const(_in const std::string& first, _in const std::string& second) {
	std::filesystem::path ret = first;
	ret = std::filesystem::absolute(ret.append(second));
	return ret.string();
}

inline std::string path_joins_const(_in const std::string& first, _in const std::vector<std::string>& seconds) {
	std::string r = first;
	for (const auto& i: seconds) r = path_join_const(r, i);
	return r;
}

// remark) it's not completely multithreaded safe.
inline std::string get_exe_dir(_option_in const char* argv0 = nullptr) {
	static bool done = false;
	static std::string ret;

	if (done) return ret;
	char name[1024] = {0,};

	// get process full path.
#ifdef _WIN32
	if (::GetModuleFileNameA(::GetModuleHandleA(NULL), name, 1023) == 0) {
		// failed?
		if (argv0) strncpy_s(name, argv0, 1023);
		else THROW_SUPUL_ERROR("fail to get_exe_dir.");
	}
#else
	if (readlink("/proc/self/exe", name, 1023) < 0) {
		// failed?
		if (argv0) {
			if (readlink(argv0, name, 1023) < 0) {
				strncpy(name, argv0, 1023);
			}
		} else {
			// need argv0.
			THROW_SUPUL_ERROR("fail to get_exe_dir.");
		}
	}
#endif
	// cache parent directory.
	ret = std::filesystem::weakly_canonical(std::filesystem::path(name)).parent_path().string();
	done = true;
	return ret;
}

// run shell command.
// return stdout.
inline std::string exec(_in const std::string& cmd, _option_out int* exitcode) {
	std::array<char, 128> buffer;
	std::string result;
	FILE* f = NULL;
	gaenari::logger::info("exec cmd: {0}", {cmd});
#ifdef _WIN32
	f = _popen(cmd.c_str(), "r");
#else
	f = popen(cmd.c_str(), "r");
#endif
	if (f == NULL) {
		*exitcode = 1;
		return "";
	}

	while (fgets(buffer.data(), static_cast<int>(buffer.size()), f) != nullptr) {
		result += buffer.data();
	}

	gaenari::logger::info("exec finished.");

#ifdef _WIN32
	int e = _pclose(f);
	*exitcode = e;
#else
	int e = pclose(f);
	*exitcode = WEXITSTATUS(e);
#endif

	return result;
}

// same as exec(cmd, exitcode).
// throw when exitcode is not 0.
inline std::string exec(_in const std::string cmd) {
	int exitcode = 0;
	auto ret = exec(cmd, &exitcode);
	if (exitcode) THROW_SUPUL_ERROR1("fail to exec, %0.", cmd);
	return ret;
}

} // common
} // supul

#endif // HEADER_GAENARI_SUPUL_COMMON_MISC_HPP
