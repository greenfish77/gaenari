// temporary code for development mode.
// it has nothing to do with the test.
//
struct develop {

inline static int main(int argc, const char** argv) try {
	// create directories and log init.
	create_directories(argc, argv, true);
	gaenari::logger::init1(log_file_path);
	gaenari::logger::warn("develop mode.");

	// run development.
	// - return weak_count_per_instance(argc, argv);
	// - return add_predict(argc, argv);
	// - return build_confusion_matrix(argc, argv);
	return report(argc, argv);
} catch(const fail& e) {
	std::string text;
	std::string w = e.what();
	gaenari::logger::error("<red>fail</red>: <yellow>" + w + "</yellow>", true);
	gaenari::logger::error("      <lmagenta>" + e.pretty_function + "</lmagenta>", true);
	gaenari::logger::error("      <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
	log_title("TEST FAILED", "red");
	return 1;
} catch(...) {
	supul::exceptions::catch_all();
	log_title("TEST FAILED", "red");
	return 1;
}

inline static int weak_count_per_instance(int argc, const char** argv) {
	// projectname : develop.
	std::string projectname = "develop";

	// delete all old project data.
	std::filesystem::remove_all(get_project_dir(projectname));

	// create project.
	create_project_test(projectname);

	// create supul object.
	auto supul_ptr = open_supul_project_for_agrawal(projectname);

	// build one small dataset.
	auto csv_path = create_agrawal_dataset(1000, 1, 0, 0.05);

	// insert and update.
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");

	// build one different small dataset, insert, update and rebuild.
	csv_path = create_agrawal_dataset(1000, 1, 1, 0.05);
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
	if (not supul_ptr->api.model.rebuild()) TEST_FAIL("fail");

	// build one different small dataset, insert, update and rebuild.
	csv_path = create_agrawal_dataset(1000, 1, 2, 0.05);
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
	if (not supul_ptr->api.model.rebuild()) TEST_FAIL("fail");

	supul_ptr->api.lifetime.close();
	return 0;
}

inline static int add_predict(int argc, const char** argv) {
	// projectname : develop.
	std::string projectname = "develop";

	// delete all old project data.
	std::filesystem::remove_all(get_project_dir(projectname));

	// create project.
	create_project_test(projectname);

	// create supul object.
	auto supul_ptr = open_supul_project_for_agrawal(projectname);

	// build one small dataset.
	auto csv_path = create_agrawal_dataset(1000, 1, 0, 0.05);

	// insert and update.
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");

	// get attributes.json.
	auto& attributes = supul_tester(*supul_ptr).get_attributes();

	// get csv path.
	int64_t total_count = 0;
	int64_t correct_count = 0;
	gaenari::dataset::for_each_csv(csv_path, ',', nullptr, [&](auto& row, auto& header_map) -> bool {
		std::unordered_map<std::string, std::string> instance;
		size_t i = 0;
		std::string y_value;
		for (const auto& it: header_map) {
			instance[it.first] = row[it.second];
			if (it.first == attributes.y) y_value = row[it.second];
		}
		auto ret = supul_ptr->api.model.predict(instance);
		if (ret.error) TEST_FAIL1("predict error: %0", ret.errormsg);

		if (y_value == ret.label) correct_count++;
		total_count++;
		return true;
	});

	return 0;
}

inline static int build_confusion_matrix(int argc, const char** argv) {
	// projectname : develop.
	std::string projectname = "develop";

	// delete all old project data.
	std::filesystem::remove_all(get_project_dir(projectname));

	// create project.
	create_project_test(projectname);

	// create supul object.
	auto supul_ptr = open_supul_project_for_agrawal(projectname);

	// build one small dataset.
	// (perb. 0.05 -> 0.9, add more noise.)
	auto csv_path = create_agrawal_dataset(100, 1, 0, 0.9);

	// insert and update.
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");

	// build another small dataset.
	csv_path = create_agrawal_dataset(100, 2, 0, 0.05);
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");

	// rebuild.
	if (not supul_ptr->api.model.rebuild()) TEST_FAIL("fail");

	return 0;
}

inline static int report(int argc, const char** argv) {
	// projectname : develop.
	std::string projectname = "develop";

	// delete all old project data.
	std::filesystem::remove_all(get_project_dir(projectname));

	// create project.
	create_project_test(projectname);

	// create supul object.
	auto supul_ptr = open_supul_project_for_agrawal(projectname);

	// good 10 chunks. (func=1)
	for (auto i=0; i<10; i++) {
		// insert and update.
		auto csv_path = create_agrawal_dataset(100, 1, i, 0.05);
		if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
		if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
	}

	// bad 10 chunks. (func=2)
	for (auto i=0; i<10; i++) {
		// insert and update.
		auto csv_path = create_agrawal_dataset(100, 2, i, 0.05);
		if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
		if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
	}

	// rebuild.
	if (not supul_ptr->api.model.rebuild()) TEST_FAIL("fail");

	// bad 10 chunks. (func=2)
	for (auto i=10; i<20; i++) {
		// insert and update.
		auto csv_path = create_agrawal_dataset(100, 2, i, 0.05);
		if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
		if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
	}

	// rebuild.
	if (not supul_ptr->api.model.rebuild()) TEST_FAIL("fail");

	// bad 10 chunks. (func=2)
	for (auto i=20; i<30; i++) {
		// insert and update.
		auto csv_path = create_agrawal_dataset(100, 2, i, 0.05);
		if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
		if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
	}

	// good 10 chunks. (func=1)
	for (auto i=10; i<20; i++) {
		// insert and update.
		auto csv_path = create_agrawal_dataset(100, 1, i, 0.05);
		if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
		if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
	}

	auto json = supul_ptr->api.report.json("");
	auto plt  = supul::supul::supul_t::api::report::gnuplot(json, {
		{"terminal",		"pngcairo"},
		{"terminal_option",	"font `Times-New-Roman,10` size 800,800"},
		{"output_filepath",	supul::common::path_join_const(temp_dir, "report.png")},
		{"plt_filepath",	supul::common::path_join_const(temp_dir, "report.plt")},
	});

	return 0;
}

};