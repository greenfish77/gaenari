#ifndef HEADER_UTIL_HPP
#define HEADER_UTIL_HPP

// utility function lists.
struct util {
	inline static void initialize(void);
	inline static void show_properties(void);
	inline static std::string get_env(_in const std::string& name);
	inline static std::string get_property_file_path(void);
	inline static bool is_alnum(_in const std::string& s);
	inline static bool is_path_extension(_in const std::string& path, _in const std::string& extension);

	inline static void initialize_config(void);
	inline static void set_config(_in const std::string& name, _in const std::string& value);
	template<typename T>
	inline static T get_config(_in const std::string& name, _in const T& def);
	inline static std::string get_config(_in const std::string& name, _in const char* def);

	template<typename T>
	inline static void check_map_has_keys(_in const T& m, _in const std::vector<std::string>& k);

	template<typename T>
	inline static std::string to_json_map_variant(_in const T& m);
};

inline std::string util::get_env(_in const std::string& name) {
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

inline std::string util::get_property_file_path(void) {
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

inline void util::initialize(void) {
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
	path.project_dir = supul::common::path_join_const(path.data_dir, "project");
	path.config_dir = supul::common::path_join_const(path.data_dir, "config");
	path.config_file_path = supul::common::path_join_const(path.config_dir, "config.txt");

	// mkdir.
	std::filesystem::create_directories(path.data_dir);
	std::filesystem::create_directories(path.project_dir);
	std::filesystem::create_directories(path.config_dir);

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

inline void util::show_properties(void) {
	gaenari::logger::info("- propereties.txt path : {0}", {path.property_file_path});
	gaenari::logger::info("- www_dir              : {0}", {path.www_dir});
	gaenari::logger::info("- data_dir             : {0}", {path.data_dir});
	gaenari::logger::info("- host                 : {0}", {option.server.host});
	gaenari::logger::info("- port                 : {0}", {option.server.port});
}

inline void util::initialize_config(void) {
	gaenari::common::prop p;
	
	// empty file.
	gaenari::common::save_to_file(path.config_file_path, "");

	// info.
	p.set("gaenari_lib_version", supul::supul::supul_t::api::misc::version());
	p.set("gaenari_web_version", GAENARI_WEB_VERSION);
	p.set("initialize_time", gaenari::common::current_yyyymmddhhmmss());
	p.save(path.config_file_path);
}

inline void util::set_config(_in const std::string& name, _in const std::string& value) {
	gaenari::common::prop p;

	if (not std::filesystem::exists(path.config_file_path)) initialize_config();
	if (not p.read(path.config_file_path)) ERROR1("fail to read %0.", path.config_file_path);
	p.set(name, value);
	p.save();
}

template<typename T>
inline T util::get_config(_in const std::string& name, _in const T& def) {
	gaenari::common::prop p;
	if (not p.read(path.config_file_path)) return def;
	return p.get(name, def);
}

inline std::string util::get_config(_in const std::string& name, _in const char* def) {
	gaenari::common::prop p;
	if (not p.read(path.config_file_path)) return def;
	return p.get(name, def);
}

inline bool util::is_alnum(_in const std::string& s) {
	return std::all_of(s.begin(), s.end(), [](char const &c) {
		return std::isalnum(c);
	});
}

// ex) path      : "/www/index.html"
//     extension : "html"
//     return true
inline bool util::is_path_extension(_in const std::string& path, _in const std::string& extension) {
	if (path.length() <= extension.length()) return false;
	if (not is_alnum(extension)) ERROR1("invalid extension: %0.", extension);
	int elength = static_cast<int>(extension.length());
	int plength = static_cast<int>(path.length());
	auto p = path.c_str();
	auto e = extension.c_str();
	if (p[plength-elength-1] != '.') return false;
	for (int i=0; i<elength; i++) {
		if (::tolower(p[plength-elength+i] != ::tolower(e[i]))) return false;
	}
	return true;
}

template<typename T>
inline void util::check_map_has_keys(_in const T& m, _in const std::vector<std::string>& k) {
	std::string msg;
	if (k.empty()) return;
	for (const auto& i: k) {
		auto find = m.find(i);
		if (find == m.end()) msg += i + ", ";
	}
	if (not msg.empty()) {
		msg.pop_back();
		msg.pop_back();
		ERROR1("keys not found: %0.", msg);
	}
}

template<typename T>
inline std::string util::to_json_map_variant(_in const T& m) {
	gaenari::common::json_insert_order_map json;

	for (const auto& it: m) {
		const auto& name = it.first;
		const supul::type::value_variant& value = it.second; 
		auto index = value.index();
		if (index == 1) json.set_value({name}, std::get<1>(value));
		else if (index == 2) json.set_value({name}, std::get<2>(value));
		else if (index == 3) json.set_value({name}, std::get<3>(value));
	}

	return json.to_string<gaenari::common::json_insert_order_map::minimize>();
}

#endif // HEADER_UTIL_HPP
