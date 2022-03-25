#ifndef HEADER_DEVELOP_UTIL_HPP
#define HEADER_DEVELOP_UTIL_HPP

// temporary code for development mode.
// it has nothing to do with the test.
//

// internal.
static inline void build_develop_sample_project_type01(_in supul_ptr& supul_ptr);
static inline void build_develop_sample_project_type02(_in supul_ptr& supul_ptr);

// inserting, updating, and rebuilding an agrawal dataset takes time.
// if there is a pre-built template, copy it.
inline void build_develop_sample_project(_in const std::string& projectname, _in int type) {
	namespace fs = std::filesystem;
	auto project_dir = get_project_dir(projectname);

	// delete all old project data.
	std::filesystem::remove_all(project_dir);

	// template existed?
	auto template_dir = supul::common::path_joins_const(temp_dir, {"project_template", "type_" + std::to_string(type)});
	if (std::filesystem::exists(template_dir)) {
		// copy and exit.
		std::filesystem::create_directories(project_dir);
		std::filesystem::copy(template_dir, project_dir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
		gaenari::logger::info("copy from template.");
		return;
	}

	// build.
	create_project_test(projectname);
	auto supul_ptr = open_supul_project_for_agrawal(projectname);
	if (type == 1) build_develop_sample_project_type01(supul_ptr);
	else if (type == 2) build_develop_sample_project_type02(supul_ptr);
	else TEST_FAIL1("invalid type: %0.", type);
	supul_ptr->api.lifetime.close();

	// copy to template.
	try {
		std::filesystem::create_directories(template_dir);
		std::filesystem::copy(project_dir, template_dir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
		gaenari::logger::info("copy to template.");
	} catch(...) {
		std::filesystem::remove_all(template_dir);
		TEST_FAIL("fail to copy.");
	}
}

// 100 instances per one chunk.
// 10(f1) -> 10(f2) -> rebuild -> 10(f2) -> rebuild -> 10(f2) -> 10(f1)
static inline void build_develop_sample_project_type01(_in supul_ptr& supul_ptr) {
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
}

// 1(1000, f1) -> 1(2000, f2) -> rebuild -> 1(1000, f2)
static inline void build_develop_sample_project_type02(_in supul_ptr& supul_ptr) {
	// 1 chunk. (func=1)
	auto csv_path = create_agrawal_dataset(1000, 1, 0, 0.05);
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");

	// 1 chunk. (func=2)
	csv_path = create_agrawal_dataset(2000, 2, 0, 0.05);
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");

	// rebuild.
	if (not supul_ptr->api.model.rebuild()) TEST_FAIL("fail");

	// 1 chunk. (func=2)
	csv_path = create_agrawal_dataset(1000, 2, 1, 0.05);
	if (not supul_ptr->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail");
	if (not supul_ptr->api.model.update()) TEST_FAIL("fail");
}

#endif // HEADER_DEVELOP_UTIL_HPP
