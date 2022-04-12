#ifndef HEADER_UTIL_HPP
#define HEADER_UTIL_HPP

struct util {
	inline static std::string get_env(_in const std::string& name) {
		std::string ret;
#ifdef _WIN32
		char* r = nullptr;
		size_t len = 0;
		if (_dupenv_s(&r, &len, name.c_str()) == 0) {
			if (r) ret = r;
		}
#else
		char* r = std::getenv(name.c_str());
		if (r) ret = r;
#endif
		return ret;
	}

	inline static std::string get_property_file_path(void) {
		auto exe_dir = supul::common::get_exe_dir();
	
		// check exe_dir.
		auto ret = supul::common::path_join_const(exe_dir, "properties.txt");
		if (std::filesystem::exists(ret)) return ret;
	
		// check exe_dir/web-root/.
		ret = supul::common::path_joins_const(exe_dir, {"web-root", "properties.txt"});
		if (std::filesystem::exists(ret)) return ret;

		// get `gaenari-web-property-file-path` env.
		ret = get_env("gaenari-web-property-file-path");
		if ((not ret.empty()) and std::filesystem::exists(ret)) return ret;

		ERROR0("properties.txt not found.");
	}

	inline static void initialize(void) {
		// get properties.txt path.
		path.property_file_path = std::filesystem::weakly_canonical(get_property_file_path()).string();
	
		// read properties.txt.
		gaenari::common::prop p;
		if (not p.read(path.property_file_path)) ERROR1("fail to read %0.", path.property_file_path);
		auto www_dir  = p.get("www_dir",  "${base_dir}/www");
		auto data_dir = p.get("data_dir", "${base_dir}/data");

		// ${base_dir} => properties.txt's base directory.
		std::filesystem::path property_file_path = path.property_file_path;
		auto base_dir = property_file_path.parent_path().string();
		gaenari::common::string_replace(www_dir,  "${base_dir}", base_dir);
		gaenari::common::string_replace(data_dir, "${base_dir}", base_dir);
		path.www_dir  = std::filesystem::weakly_canonical(www_dir).string();
		path.data_dir = std::filesystem::weakly_canonical(data_dir).string();
		auto tmp = get_env("gaenari-web-data-dir");
		if (not tmp.empty()) path.data_dir = tmp;

		// mkdir.
		std::filesystem::create_directories(path.data_dir);

		// set log.
		path.log_file_path = supul::common::path_join_const(path.data_dir, "log");
		std::filesystem::create_directories(path.log_file_path);
		path.log_file_path = supul::common::path_join_const(path.log_file_path, "log.txt");
		gaenari::logger::init1(path.log_file_path);

		// check.
		if (not std::filesystem::exists(path.www_dir)) ERROR1("www_dir is not found: %0.", path.www_dir);

		// read options.
		option.server.host = p.get("host", "");
		option.server.port = p.get("port", 0);
		if (option.server.host.empty() or (option.server.port == 0)) ERROR2("invald host=%0, port=%1.", option.server.host, option.server.port);
	}

	inline static void show_properties(void) {
		gaenari::logger::info("- propereties.txt path : {0}", {path.property_file_path});
		gaenari::logger::info("- www_dir              : {0}", {path.www_dir});
		gaenari::logger::info("- data_dir             : {0}", {path.data_dir});
		gaenari::logger::info("- host                 : {0}", {option.server.host});
		gaenari::logger::info("- port                 : {0}", {option.server.port});
	}
};

#endif // HEADER_UTIL_HPP