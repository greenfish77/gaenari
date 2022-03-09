#ifndef HEADER_UTIL_HPP
#define HEADER_UTIL_HPP

// utils.
//  - is_develop_mode()
//	- create_directories()
//	- log_title()
//	- exec()
//	- get_sha1()
//	- http_download()
//	- create_agrawal_dataset()

// if the <exe_dir/develop.txt> file exists in debug, development.hpp is run instead of test.
inline bool is_develop_mode(void) {
#ifdef NDEBUG
	return false;
#else
	auto check_file_path = supul::common::path_join_const(supul::common::get_exe_dir(), "develop.txt");
	if (std::filesystem::exists(check_file_path)) return true;
	return false;
#endif
}

inline void create_directories(_in const int argc, _in const char** argv, _in bool remove_all) {
	// get test base directory. (%exe_dir%/test_dir)
	// clean directory, and initialize logger.
	auto exe_dir		= supul::common::get_exe_dir(argv[0]);
	auto test_base_dir	= supul::common::path_join_const(exe_dir,		"test_dir");
	auto test_dir		= supul::common::path_join_const(test_base_dir,	"test");
	csv_dir				= supul::common::path_join_const(test_base_dir,	"csv");
	download_dir		= supul::common::path_join_const(test_base_dir,	"download");
	temp_dir			= supul::common::path_join_const(test_base_dir,	"temp");
	log_file_path		= supul::common::path_join_const(test_dir,		"_log.txt");
	weka_jar_path		= supul::common::path_join_const(download_dir,	"weka-stable-3.6.6.jar");
	project_base_dir	= supul::common::path_join_const(test_dir,		"projects");
	if (remove_all) std::filesystem::remove_all(test_dir);
	std::filesystem::create_directories(test_dir);
	std::filesystem::create_directories(csv_dir);
	std::filesystem::create_directories(download_dir);
	std::filesystem::create_directories(project_base_dir);
	std::filesystem::create_directories(temp_dir);
}

// show title.
// ------...- (52 characters)
// title
// ------...-
inline void log_title(_in const std::string& title, _in const std::string color="", _in const std::string bar_color="") {
	std::string bar(52, '=');
	if (bar_color.empty()) {
		if (not color.empty()) bar = "<" + color + '>' + bar + "</" + color + '>';
	} else {
		bar = "<" + bar_color + '>' + bar + "</" + bar_color + '>';
	}
	gaenari::logger::info(bar, true);
	if (color.empty()) gaenari::logger::info(title);
	else gaenari::logger::info("<" + color + '>' + title + "</" + color + '>', true);
	gaenari::logger::info(bar, true);
}

// get sha1 from file.
inline std::string get_sha1(_in const std::string& file_path) {
	std::string ret;
	std::string cmd;

	// get sha1 by cmd.
#ifdef _WIN32
	cmd = "certutil -hashfile \"" + file_path +"\" sha1 | find /i /v \"sha1\" | find /i /v \"certutil\"";
#else
	cmd = "sha1sum '" + file_path + "'| awk '{print $1}'";
#endif

	// execute.
	ret = supul::common::exec(cmd);

	// trim right, and make upper.
	return gaenari::common::str_upper_const(gaenari::common::trim_right(ret, "\r\n"));
}

// download file.
// http_download("http://www.google.com", "index.html", "...");
inline void http_download(_in const std::string& url, _in const std::string filename, _in const std::string& sha1) {
	// file exits check.
	// and
	// download.
	
	// check file.
	auto path = supul::common::path_join_const(download_dir, filename);
	if (std::filesystem::exists(path)) {
		// check sha1.
		if (sha1 == get_sha1(path)) {
			gaenari::logger::info("download file already exists: {0}.", {path});
			return;
		}
	}
	
	// curl.
	std::string cmd = "curl -o \"" + path + "\" " + url + " --progress-bar";
	try {
		supul::common::exec(cmd);
	} catch (...) {
		supul::exceptions::catch_all();
		gaenari::logger::warn("download failed.");
		gaenari::logger::warn("check network connection or proxy settings.");
#ifdef _WIN32
		gaenari::logger::warn("...> SET http_proxy=http://xxx.xxx.xxx.xxx:xxxx");
		gaenari::logger::warn("...> SET https_proxy=http://xxx.xxx.xxx.xxx:xxxx");
		gaenari::logger::warn("...> tests.exe");
#else
		gaenari::logger::warn("$ export http_proxy=http://xxx.xxx.xxx.xxx:xxxx");
		gaenari::logger::warn("$ export https_proxy=http://xxx.xxx.xxx.xxx:xxxx");
		gaenari::logger::warn("$ ./tests");
#endif
		TEST_FAIL("download fail.");
	}

	// get sha1.
	auto sha1_download = get_sha1(path);
	if (sha1 != sha1_download) {
		TEST_FAIL("sha1 mis-matched. download=" + sha1_download + ", verification="+sha1);
	}
	gaenari::logger::info("download completed.");
}

inline std::string get_agrawal_dataset_filepath(_in int instances, _in int func, _in int seed, _in double perturbation, _in const std::string& extension_without_dot) {
	if ((func < 0) or (func > 10)) TEST_FAIL("invalid func: " + std::to_string(func));
	if ((perturbation < 0.0) or (perturbation > 1.0)) TEST_FAIL("invalid pertubation: " + std::to_string(perturbation));

	// name : agrawal-i<instances>-s<seed>-f<func>-p<pert>.csv
	std::string name = gaenari::common::f("agrawal-i%0-f%1-s%2-p%3", {instances, func, seed, gaenari::common::dbl_to_string(perturbation)});

	// return full path.
	return supul::common::path_join_const(csv_dir, name + '.' + extension_without_dot);
}

// create agrawal dataset by weka framework.
// - inst : the row instance count.
// - func : the function to use for generating the data.   (1 <= func <= 10.)
// - seed : the random seed value.
// - pert : the perturbation fraction(fluctuating degree). (0 <= pert <= 1.0). default (0.05).
// - ret  : csv file full path.
//			<csv_dir>/agrawal-i<instances>-f<func>-s<seed>-p<pert>.csv
inline std::string create_agrawal_dataset(_in int instances, _in int func, _in int seed, _in double perturbation) {
	std::string cmd;

	// get arff and csv file path.
	std::string arff_path		= get_agrawal_dataset_filepath(instances, func, seed, perturbation, "tmp.arff");
	std::string csv_path		= get_agrawal_dataset_filepath(instances, func, seed, perturbation, "tmp.csv");
	std::string ret_csv_path	= get_agrawal_dataset_filepath(instances, func, seed, perturbation, "csv");

	// check.
	if (not ((1 <= func) and (func <= 10)))						TEST_FAIL1("invalid agrawal func(%0).", func);
	if (not ((0.0 <= perturbation) and (perturbation <= 1.0)))	TEST_FAIL1("invalild agrawal perturbation(%0).", perturbation);

	// already existed.
	if (std::filesystem::exists(ret_csv_path)) {
		// do not check sha1.
		return ret_csv_path;
	}

	// remove.
	std::filesystem::remove(arff_path);
	std::filesystem::remove(csv_path);

	// build arff file.
	cmd =	"java -classpath \"" + weka_jar_path + "\" "
			"weka.datagenerators.classifiers.classification.Agrawal "
			"-r temp "
			"-S " + std::to_string(seed)		+ ' ' +
			"-n " + std::to_string(instances)	+ ' ' +
			"-F " + std::to_string(func)		+ ' ' +
			"-P " + gaenari::common::dbl_to_string(perturbation) + ' ' +
			" > \"" + arff_path + '\"';
	supul::common::exec(cmd);

	// arff -> csv.
	cmd =	"java -classpath \"" + weka_jar_path + "\" "
			"weka.core.converters.CSVSaver "
			"-i \"" + arff_path + "\" "
			"-o \"" + csv_path  + '\"';
	supul::common::exec(cmd);

	// remove arff.
	std::filesystem::remove(arff_path);

	// rename csv.
	std::filesystem::rename(csv_path, ret_csv_path);

	return ret_csv_path;
}

// create attributes.json for agrawal dataset.
// {
// 	"revision": 0,
// 	"fields": {
// 		"salary":		"REAL",
// 		"commission":	"REAL",
// 		"age":			"INTEGER",
// 		"elevel":		"TEXT_ID",
// 		"car":			"TEXT_ID",
// 		"zipcode":		"TEXT_ID",
// 		"hvalue":		"REAL",
// 		"hyears":		"INTEGER",
// 		"loan":			"REAL",
// 		"group":		"TEXT_ID"
// 	},
// 	"x": ["salary", "commission", "age", "elevel", "car", "zipcode", "hvalue", "hyears", "loan"],
// 	"y": "group"
// }
inline void create_agrawal_attributes_json(_in const std::string& base_dir) {
	bool error = false;
	using supul_t = supul::supul::supul_t;
	error |= not supul_t::api::project::add_field(base_dir,	"salary",		"REAL");
	error |= not supul_t::api::project::add_field(base_dir, "commission",	"REAL");
	error |= not supul_t::api::project::add_field(base_dir, "age",			"INTEGER");
	error |= not supul_t::api::project::add_field(base_dir, "elevel",		"TEXT_ID");
	error |= not supul_t::api::project::add_field(base_dir, "car",			"TEXT_ID");
	error |= not supul_t::api::project::add_field(base_dir, "zipcode",		"TEXT_ID");
	error |= not supul_t::api::project::add_field(base_dir, "hvalue",		"REAL");
	error |= not supul_t::api::project::add_field(base_dir, "hyears",		"INTEGER");
	error |= not supul_t::api::project::add_field(base_dir, "loan",			"REAL");
	error |= not supul_t::api::project::add_field(base_dir, "group",		"TEXT_ID");

	error |= not supul_t::api::project::x(base_dir, {"salary", "commission", "age", "elevel", "car", "zipcode", "hvalue", "hyears", "loan"});
	error |= not supul_t::api::project::y(base_dir, "group");

	if (error) TEST_FAIL("fail to project setting.");

	// is attributes.json good?
	auto json_path = supul::common::path_joins_const(base_dir, {"conf", "attributes.json"});
	if (get_sha1(json_path) != "2C20E0EC57A42E267A4064BFCA0920AB6AB10797") TEST_FAIL("json file is invalid.");
}

template<typename real_t>
inline bool is_approximate_equal(_in real_t a, _in real_t b) {
	// https://stackoverflow.com/a/41405501
	constexpr auto tol = std::numeric_limits<real_t>::epsilon();
	real_t diff = std::fabs(a - b);
	if (diff <= tol) return true;
	if (diff < std::fmax(std::fabs(a), std::fabs(b)) * tol) return true;
	return false;
}

inline std::vector<std::string> get_test_scenario_names(_in int argc, _in const char** argv) {
	if ((argc <= 0) or (not argv)) TEST_FAIL("invalid cmdline.");
	if (argc == 1) return {"default", "predict"};
	
	std::vector<std::string> r;
	for (int i=1; i<argc; i++) {
		auto name = argv[i];
		if (not name) TEST_FAIL("invalid cmdline.");
		r.emplace_back(gaenari::common::str_lower_const(name));
	}

	return r;
}

inline std::string get_project_dir(_in const std::string& projectname) {
	return supul::common::path_join_const(project_base_dir, projectname);
}

// http://www.cse.yorku.ca/~oz/hash.html
inline unsigned int simple_string_hash(_in const std::string& s) {
	unsigned int hash = 5381;

	for (size_t i=0; i<s.length(); i++) {
		hash = ((hash << 5) + hash) + s.c_str()[i];
	}

	return hash;
}

#endif // HEADER_UTIL_HPP
