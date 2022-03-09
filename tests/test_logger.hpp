#ifndef HEADER_TEST_LOGGER_HPP
#define HEADER_TEST_LOGGER_HPP

class test_logger {
public:
	test_logger() = delete;
	test_logger(_in const std::string& category, _in const std::string& name, _option_in bool show_line = true) {
		test_category = category;
		test_name = name;
		std::string msg  = "<yellow>" + category + " STARTED: " + name + "</yellow>";
		std::string line = "<yellow>" + std::string(msg.length() - 17, '-') + "</yellow>";
		if (show_line) gaenari::logger::info(line, true);
		gaenari::logger::info(msg,  true);
		if (show_line) gaenari::logger::info(line, true);
		_show_line = show_line;
	}
	void failed(void) {
		_failed = true;
	}
	bool is_failed(void) {
		return _failed;
	}
	~test_logger(void) {
		std::string msg, line;
		if (_failed) {
			msg  = "<red>" + test_category + " FAILED: " + test_name + "</red> (" + t.to_string() + ')';
			line = "<red>" + std::string(msg.length() - 11, '-') + "</red>";
		} else {
			msg  = "<yellow>" + test_category + " PASSED: " + test_name + "</yellow> (" + t.to_string() + ')';
			line = "<yellow>" + std::string(msg.length() - 17, '-') + "</yellow>";
		}
		if (_show_line) gaenari::logger::info(line, true);
		gaenari::logger::info(msg, true);
		if (_show_line) gaenari::logger::info(line, true);
	}
protected:
	gaenari::common::elapsed_time t;
	std::string test_name;
	std::string test_category;
	bool _failed = false;
	bool _show_line = true;
};

// do success test.
// TESTCASE_OK(testname, function, arguments)
// if the function fails(raised exception), it throws.
//
// ex)
// void test(int& a, double b) {...}
// int c = 0;
// TESTCASE_OK("test1", test, c, 3.14);
template<typename T, typename... Args>
inline void TESTCASE_OK(_in const std::string& name, _in const T& function, _in Args&&... args) {
	test_logger l{"TESTCASE_OK", name};
	try {
		function(args...);
	} catch (const base_error& e) {
		l.failed();
		std::string text;
		std::string w = e.what();
		gaenari::logger::error("<red>test failed</red>: <yellow>" + w + "</yellow>", true);
		gaenari::logger::error("             <lmagenta>" + e.pretty_function + "</lmagenta>", true);
		gaenari::logger::error("             <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
	} catch(...) {
		l.failed();
		supul::exceptions::catch_all();
	}
	if (l.is_failed()) TEST_FAIL("TEST FAILED.");
}

// do fail test.
// TESTCASE_NG(testname, function, arguments)
// if the function succeeds, it throws.
//
// ex)
// void test(int& a, double b) {...}
// int c = 0;
// TESTCASE_NG("test2", test, c, 2.48);
template<typename T, typename... Args>
inline void TESTCASE_NG(_in const std::string& name, _in const T& function, _in Args&&... args) {
	test_logger l{"TESTCASE_NG", name};
	try {
		function(args...);
		l.failed();
		gaenari::logger::error("no excpetion raised.");
	} catch (const std::exception& e) {
		std::string w = e.what();
		gaenari::logger::info("exception: " + w);
		gaenari::logger::info("raised exception, but test success.");
	}
	if (l.is_failed()) TEST_FAIL("TEST FAILED.");
}

#endif // HEADER_TEST_LOGGER_HPP
