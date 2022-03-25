// include all gaenari and supul header files.
#include "gaenari/gaenari.hpp"

// the test program is simple.
// use globals.
std::string download_dir;
std::string log_file_path;
std::string weka_jar_path;
std::string csv_dir;
std::string project_base_dir;
std::string temp_dir;

// include headers for tests.
#include "exceptions.hpp"
#include "util.hpp"
#include "tests.hpp"
#include "test_logger.hpp"
#include "testcase.hpp"
#include "testscenario.hpp"
#include "develop_util.hpp"
#include "_develop.hpp"

int main(int argc, const char** argv) try
{
#ifndef NDEBUG
	// in debug, do develop mode if set.
	if (is_develop_mode()) { return develop::main(argc, argv); }
#endif

	// create directories and log init.
	create_directories(argc, argv, true);
	gaenari::logger::init1(log_file_path);
	gaenari::common::elapsed_time t;

	// start.
	log_title("TEST STARTED", "yellow");

	// get test scenario names.
	auto scenario_names = get_test_scenario_names(argc, argv);

	// check system.
	TESTCASE_OK("ready_test", ready_test);

	// do test.
	for (const auto& scenario_name: scenario_names) {
		if		(scenario_name == "default")		scenario_default("default");
		else if (scenario_name == "large")			scenario_largesize("large");
		else if (scenario_name == "predict")		scenario_predict("predict");
		else if (scenario_name == "limit_chunk")	scenario_limit_chunk("limit_chunk");
		else	TEST_FAIL1("invalid scenario name: %0", scenario_name);
	}

	// finished.
	log_title(F("TEST SUCCESS (%0)", {t.to_string()}), "yellow");

	return 0;
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

// under windows, memory leak can be reported.
#ifdef _WIN32
static struct DUMP_MEMORY_LEAK {DUMP_MEMORY_LEAK() {_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);}} g_dml;
#endif
