#ifndef HEADER_UNIT_TEST_HPP
#define HEADER_UNIT_TEST_HPP

// unit test apis.
//	- ready_test()
//	- create_project_test()
//	- insert_update_test()
//	- rebuild_test()

// is it a testable environment?
inline void ready_test(void) {
	// required commands. (common linux and windows)
	std::vector<std::string> cmds = {"curl", "java"};

	// in development, skip this.
	if (is_develop_mode()) return;

	// required commands.
	std::string checker;
#ifdef _WIN32
	checker = "where ";
	cmds.push_back("certutil");
#else
	checker = "command -v ";
	cmds.push_back("sha1sum");
#endif

	// do check.
	try {
		for (const auto& cmd: cmds) {
			auto full_cmd = checker + cmd;
			supul::common::exec(full_cmd);
		}
	} catch(...) {
		gaenari::logger::error("install the required programs and add them to the path.");
		supul::exceptions::catch_all();
		TEST_FAIL("install the required programs.");
	}

	// download stuff.
	http_download("https://repo1.maven.org/maven2/nz/ac/waikato/cms/weka/weka-stable/3.6.6/weka-stable-3.6.6.jar", "weka-stable-3.6.6.jar", "EB67B46F093895BBB60837094E4B1995F466193C");
}

// create project.
inline void create_project_test(_in const std::string& projectname) {
	create_supul_project_for_agrawal(projectname);
}

// supul insert and update with loop.
inline void insert_update_test(_in const std::string& projectname, _in int trials, _in int instances, _in int func, _in int start_seed, _in double perturbation, _in int64_t expected_instance_correct_count) {
	// open project.
	auto supul = open_supul_project_for_agrawal(projectname);
	
	// get db.
	auto& db = supul_tester(*supul).get_db();

	// get global.
	auto global_before = db.get_global();

	// seed.
	int seed = start_seed;

	for (int i=0; i<trials; i++) {
		// insert chunks.
		// each time a chunk with different seed value is inserted.
		// so there is no concept drift.
		insert_agrawal_chunk(supul, instances, func, seed, perturbation);

		// next seed.
		seed++;

		// after insert, do update.
		supul->api.model.update();
	}

	// get global for validation.
	auto global_after = db.get_global();
	auto before_instance_count	= supul::common::get_variant_int64(global_before,"instance_count");
	auto instance_count			= supul::common::get_variant_int64(global_after, "instance_count");
	auto updated_instance_count	= supul::common::get_variant_int64(global_after, "updated_instance_count");
	auto instance_correct_count	= supul::common::get_variant_int64(global_after, "instance_correct_count");
	auto instance_accuracy		= supul::common::get_variant_double(global_after,"instance_accuracy");
	if (updated_instance_count == 0) TEST_FAIL("update_instance_count is zero.");
	auto instance_accuracy_calc	= static_cast<double>(instance_correct_count) / static_cast<double>(updated_instance_count);

	// test.
	int64_t trials_instance = static_cast<int64_t>(trials) * static_cast<int64_t>(instances);
	if (instance_count != trials_instance + before_instance_count)			 TEST_FAIL2("fail: %0 != %1.", instance_count, trials_instance + before_instance_count);
	if (updated_instance_count != instance_count)							 TEST_FAIL2("fail: %0 != %1.", updated_instance_count, instance_count);
	if (instance_correct_count != expected_instance_correct_count)			 TEST_FAIL2("fail: %0 != %1.", expected_instance_correct_count, instance_correct_count);
	if (not is_approximate_equal(instance_accuracy, instance_accuracy_calc)) TEST_FAIL2("fail: %0 != %1.", instance_accuracy, instance_accuracy_calc);

	// verify all.
	auto& model = supul_tester(*supul).get_model();
	model.verify_all();
}

// rebuild test.
inline void rebuld_test(_in const std::string& projectname, _in int64_t expected_instance_correct_count) {
	// open project.
	auto supul = open_supul_project_for_agrawal(projectname);

	// get db.
	auto& db = supul_tester(*supul).get_db();

	// get global.
	auto global_before = db.get_global();

	// do rebuild.
	if (not supul->api.model.rebuild()) TEST_FAIL("fail to supul.api.rebuild().");

	// get global for validation.
	auto global_after = db.get_global();
	auto before_instance_count			= supul::common::get_variant_int64(global_before, "instance_count");
	auto before_updated_instance_count	= supul::common::get_variant_int64(global_before, "updated_instance_count");
	auto before_instance_correct_count	= supul::common::get_variant_int64(global_before, "instance_correct_count");
	auto before_instance_accuracy		= supul::common::get_variant_double(global_before,"instance_accuracy");

	auto after_instance_count			= supul::common::get_variant_int64(global_after, "instance_count");
	auto after_updated_instance_count	= supul::common::get_variant_int64(global_after, "updated_instance_count");
	auto after_instance_correct_count	= supul::common::get_variant_int64(global_after, "instance_correct_count");
	auto after_instance_accuracy		= supul::common::get_variant_double(global_after,"instance_accuracy");

	if ((after_updated_instance_count == 0) or (before_updated_instance_count == 0)) TEST_FAIL("update_instance_count is zero.");
	auto before_instance_accuracy_calc	= static_cast<double>(before_instance_correct_count) / static_cast<double>(before_updated_instance_count);
	auto after_instance_accuracy_calc	= static_cast<double>(after_instance_correct_count)  / static_cast<double>(after_updated_instance_count);

	// log.
	gaenari::logger::info("rebuild effect: accuracy {0} -> {1}.", {before_instance_accuracy_calc, after_instance_accuracy_calc});

	// test.
	if (after_instance_correct_count != expected_instance_correct_count) TEST_FAIL2("fail: expected_instance_correct_count(%0) != after_instance_correct_count(%1).", expected_instance_correct_count, after_instance_correct_count);
	if (not is_approximate_equal(before_instance_accuracy, before_instance_accuracy_calc)) TEST_FAIL2("fail: before_instance_accuracy(%0) != before_instance_accuracy_calc(%1).", before_instance_accuracy, before_instance_accuracy_calc);
	if (not is_approximate_equal(after_instance_accuracy,  after_instance_accuracy_calc))  TEST_FAIL2("fail: after_instance_accuracy(%0) != after_instance_accuracy_calc(%1).", after_instance_accuracy,  after_instance_accuracy_calc);
	if (before_instance_count != after_instance_count) TEST_FAIL2("fail: before_instance_count(%0) != after_instance_count(%1).", before_instance_count, after_instance_count);

	// verify all.
	auto& model = supul_tester(*supul).get_model();
	model.verify_all();
}

inline void chunk_initial_accuray_test(_in const std::string& projectname, _in const std::vector<double>& expected_accuracies) {
	// open project.
	auto supul = open_supul_project_for_agrawal(projectname);

	// get db.
	auto& db = supul_tester(*supul).get_db();

	// get accuracies.
	auto accuracies = db.get_chunk_initial_accuracy();

	// check size.
	if (accuracies.size() != expected_accuracies.size()) TEST_FAIL2("chunk size not matched: %0 != %1.", expected_accuracies.size(), accuracies.size());

	// compare.
	for (size_t i=0; i<accuracies.size(); i++) {
		if (not is_approximate_equal(expected_accuracies[i], accuracies[i])) {
			TEST_FAIL3("accuracy is not matched, index=%0, expected=%1, tested=%2.", i, expected_accuracies[i], accuracies[i]);
		}
	}
}

inline void report_test(_in const std::string& projectname, _in int64_t expected_json_string_hash, _in int64_t expected_gnuplot_script_hash) {
	// open project.
	auto supul = open_supul_project_for_agrawal(projectname);

	// get report.
	// if `datetime` is included, the hash is always changed, so change it.
	auto json = supul->api.report.json("{\"datetime_as_index\":true}");

	auto hash_json = simple_string_hash(json);
	if (static_cast<unsigned int>(expected_json_string_hash) != hash_json) TEST_FAIL3("not matched report json result hash, value(%0), expected(%1),\n**value**\n%2", hash_json, expected_json_string_hash, json);

	// get gnuplot script.
	// do not save any files(png, plt).
	auto gnuplot = supul->api.report.gnuplot(json, {
		{"terminal", "dumb"},
	});

	auto hash_gnuplot = simple_string_hash(gnuplot.value());
	if (static_cast<unsigned int>(expected_gnuplot_script_hash) != hash_gnuplot) TEST_FAIL3("not matched report gnuplot result hash, value(%0), expected(%1),\n**value**\n%2", hash_gnuplot, expected_gnuplot_script_hash, gnuplot.value());
}

// predict test.
inline void predict_test(_in const std::string& projectname, _in int instances, _in const std::vector<int>& funcs) {
	int64_t total_count = 0;
	int64_t correct_count = 0;
	int64_t error_count = 0;

	// open project.
	auto supul = open_supul_project_for_agrawal(projectname);

	// get db.
	auto& db = supul_tester(*supul).get_db();

	for (const auto& func: funcs) {
		auto csv_path = get_agrawal_dataset_filepath(instances, func, 0, 0.05, "csv");
		auto result   = predict_csv(supul, csv_path);
		total_count   += result.total_count;
		correct_count += result.correct_count;
		error_count   += result.error_count;
	}

	gaenari::logger::info(">>>> total:   {0}", {total_count});
	gaenari::logger::info(">>>> correct: {0}", {correct_count});
	gaenari::logger::info(">>>> error:   {0}", {error_count});

	// get global.
	auto global = db.get_global();
	auto global_updated_instance_count = supul::common::get_variant_int64(global, "updated_instance_count");
	auto global_instance_correct_count = supul::common::get_variant_int64(global, "instance_correct_count");
	if (total_count   != global_updated_instance_count) TEST_FAIL2("fail to total_count(%0) != global_updated_instance_count(%1)", total_count, global_updated_instance_count);
	if (correct_count != global_instance_correct_count) TEST_FAIL2("fail to correct_count(%0) != global_instance_correct_count(%1)", correct_count, global_instance_correct_count);
	if (error_count != 0) TEST_FAIL1("error count: %0.", error_count);
	gaenari::logger::info("predict_test count matched.");
}

// predict test with initial_accuray of chunk.
inline void predict_test2(_in const std::string& projectname, _in int instances, _in int last_func) {
	// open project.
	auto supul = open_supul_project_for_agrawal(projectname);

	// get db.
	auto& db = supul_tester(*supul).get_db();

	// test one csv.
	auto csv_path = get_agrawal_dataset_filepath(instances, last_func, 0, 0.05, "csv");
	auto result   = predict_csv(supul, csv_path);

	gaenari::logger::info(">>>> total:   {0}", {result.total_count});
	gaenari::logger::info(">>>> correct: {0}", {result.correct_count});
	gaenari::logger::info(">>>> error:   {0}", {result.error_count});

	// get last chunk id.
	auto id = db.get_chunk_last_id();
	gaenari::logger::info("last chunk id: {0}.", {id});
	auto chunk = db.get_chunk_by_id(id);
	auto initial_accuracy		= supul::common::get_variant_double(chunk, "initial_accuracy");
	auto initial_correct_count	= supul::common::get_variant_int64(chunk, "initial_correct_count");
	auto total_count			= supul::common::get_variant_int64(chunk, "total_count");
	gaenari::logger::info("initial_accuracy: {0}.", {initial_accuracy});

	// test.
	if (total_count != result.total_count) TEST_FAIL2("total_count(%0) != result.total_count(%1).", total_count, result.total_count);
	if (initial_correct_count != result.correct_count) TEST_FAIL2("initial_correct_count(%0) != result.correct_count(%1).", initial_correct_count, result.correct_count);
	gaenari::logger::info("predict_test2 count matched.");
}

#endif // HEADER_UNIT_TEST_HPP
