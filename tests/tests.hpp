#ifndef HEADER_TESTS_HPP
#define HEADER_TESTS_HPP

// gaenari, supul tests.
// - create_supul_project_for_agrawal()
// - open_supul_project_for_agrawal()
// - insert_agrawal_chunk()

namespace supul::supul {
// supul_tester.
// access protected things.
class supul_tester {
public:
	supul_tester() = delete;
	inline supul_tester(supul_t& supul): supul{supul} {}
public:
	// return protected variables.
	inline bool initialized(void) {return supul.api.initialized;}
	inline gaenari::common::prop& property(void) {return supul.prop;}
	inline auto& get_db(void) {return supul.model.get_db();}
	inline auto& get_model(void) {return supul.model;}
	inline auto& get_attributes(void) {return supul.attributes;}
public:
	supul_t& supul;
};
}
using supul_tester = supul::supul::supul_tester;

// supul class has no class assign constructor.
// can not `supul::supul::supul any_function(...)`.
// so use pointer.
using supul_ptr = std::unique_ptr<supul::supul::supul_t>;

// create supul project.
// set supul directory as <supul_base_dir>/<projectname>.
inline supul_ptr create_supul_project_for_agrawal(_in const std::string& projectname) {
	auto supul = std::make_unique<supul::supul::supul_t>();

	// get supul directory.
	auto project_dir = get_project_dir(projectname);

	// create project.
	if (not supul::supul::supul_t::api::project::create(project_dir)) TEST_FAIL("fail to create project.");

	// get some paths of project.
	supul::type::paths paths;
	supul::common::get_paths(project_dir, paths);

	// db.type = sqlite.
	if (not supul::supul::supul_t::api::project::set_property(project_dir, "db.type", "sqlite")) TEST_FAIL("fail to project::set_property.");

	// modify attributes.json for agrawal.
	create_agrawal_attributes_json(paths.base_dir);

	// all done, try to init.
	if (not supul->api.lifetime.open(project_dir)) TEST_FAIL("fail to supul.api.init().");

	return supul;
}

// open project.
inline supul_ptr open_supul_project_for_agrawal(_in const std::string& projectname) {
	auto supul = std::make_unique<supul::supul::supul_t>();

	// set supul directory.
	auto project_dir = supul::common::path_join_const(project_base_dir, projectname);

	// init.
	if (not supul->api.lifetime.open(project_dir)) TEST_FAIL("fail to supul.api.init().");
	return supul;
}

// agrawal dataset insert.
inline void insert_agrawal_chunk(_in supul_ptr& supul, _in int instances, _in int func, _in int seed, _in double perturbation=0.05) {
	// create data.csv
	auto csv_path = create_agrawal_dataset(instances, func, seed, perturbation);
	if (not supul->api.model.insert_chunk_csv(csv_path)) TEST_FAIL("fail to supul.api.insert_chunk_csv().");
}

inline auto predict_csv(_in supul_ptr& supul, _in const std::string& csv_path) {
	struct {
		int64_t total_count = 0;
		int64_t correct_count = 0;
		int64_t error_count = 0;
	} ret;

	// get attributes.json.
	auto& attributes = supul_tester(*supul).get_attributes();

	// predict instances in csv.
	if (not gaenari::dataset::for_each_csv(csv_path, ',', nullptr, [&](auto& row, auto& header_map) -> bool {
		std::unordered_map<std::string, std::string> instance;
		size_t i = 0;
		std::string ground_truth;
		for (const auto& it: header_map) {
			instance[it.first] = row[it.second];
			if (it.first == attributes.y) ground_truth = row[it.second];
		}
		auto predict_result = supul->api.model.predict(instance);
		if (predict_result.error) ret.error_count++;
		else if (ground_truth == predict_result.label) ret.correct_count++;
		ret.total_count++;
		return true;
	})) TEST_FAIL1("fail to for_each_csv %0.", csv_path);

	return ret;
}

#endif // HEADER_TESTS_HPP
