#ifndef HEADER_TESTSCENARIO_HPP
#define HEADER_TESTSCENARIO_HPP

// create_project -> insert 10 chunks -> insert 10 chunks of changed data -> rebuild -> insert 10 chunks
inline void scenario_default(_in const std::string& projectname) {
	// create project.
	TESTCASE_OK("create_project", create_project_test, projectname);

	// test parameter.
	int trials		= 10;	// 10 trials. after calling insert_update_test(), trials * instance = 10,000 instances are added.
	int instances	= 1000;	// one chunk has 1,000 instances.
	int func		= 1;	// agrawal data generation function. as func changes, the trend of the data changes. (0<=func<=10)
	int start_seed	= 0;	// starting random seed value. in insert_update_test(), +1 seed for each chunks.
	double pert		= 0.05;	// perturbation value for generating agrawal data. default 0.05.

	// no_concept_drifting. (agrawal func=1). accuracy = 0.9395.
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, start_seed, pert, 9395);

	// concept_drifted. (agrawal func=2). accuracy = 0.7009.
	// because the agrawal function has changed, the trend of the data has changed.
	// so accuracy decreased.
	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, start_seed, pert, 14018);

	// rebuild to increase accuracy.
	// after rebuild, accuracy is slightly improved. (0.7009 -> 0.7262)
	TESTCASE_OK("rebuild", rebuld_test, projectname, 14524);

	// check the accuracy after adding 10 chunks(func=2).
	// accuracy = 0.7245 (21735/30000)
	start_seed = trials;
	TESTCASE_OK("after_rebuild", insert_update_test, projectname, trials, instances, func, start_seed, pert, 21735);

	// rebuild again.
	// since there are still 10000 instances of func=1, 0.9 is not reached.
	// accuracy : 0.7245 -> 0.8124.
	TESTCASE_OK("rebuild", rebuld_test, projectname, 24371);

	// all initial accuracies of chunks test.
	// this is an accuracy record for 30 chunks.
	std::vector<double> expected_initial_accuracy = {
		0.988, 0.938, 0.93,  0.926, 0.934, 0.934, 0.928, 0.938, 0.938, 0.941,	// a) func1. avg=0.9395. same func.		kept in high accuracy. (first chunk(0.988) trained).
		0.474, 0.487, 0.506, 0.46,  0.457, 0.441, 0.447, 0.461, 0.444, 0.446,	// b) func2. avg=0.4623. func changed.	decrease in accuracy.
																				// c) rebuild.							func1: 50%, func2: 50%. accuracy = 0.7009 -> 0.7262.
		0.73,  0.741, 0.725, 0.723, 0.727, 0.727, 0.701, 0.715, 0.71,  0.712	// d) func2. avg=0.7211.				increased 0.4623 -> 0.7211.
																				// e) rebuild.							func1: 33%, func2: 66%. accuracy = 0.7245 -> 0.8124.
	};
	TESTCASE_OK("initial_accuracy", chunk_initial_accuray_test, projectname, expected_initial_accuracy);

	// report test.
	TESTCASE_OK("report test", report_test, projectname, 2750664466, 3940813681);
}

inline void scenario_largesize(_in const std::string& projectname) {
	// create project.
	TESTCASE_OK("create_project", create_project_test, projectname);

	// test parameter.
	int trials		= 10;
	int instances	= 100000;
	int func		= 1;
	int start_seed	= 0;
	double pert		= 0.05;

	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, start_seed, pert, 939800);
	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, start_seed, pert, 1398461);
	TESTCASE_OK("rebuild", rebuld_test, projectname, 1446770);
	start_seed = trials;
	TESTCASE_OK("after_rebuild", insert_update_test, projectname, trials, instances, func, start_seed, pert, 2222158);
	TESTCASE_OK("rebuild", rebuld_test, projectname, 2421192);
}

inline void scenario_predict(_in const std::string& projectname) {
	// create project
	TESTCASE_OK("create_project", create_project_test, projectname);

	// test parameter.
	int trials		= 1;		// 1 trials. (must be one in scenario_predict.)
	int instances	= 10000;	// one chunk has 10,000 instances.
	int func		= 1;		// 
	int start_seed	= 0;		// must be zero in scenario_predict.
	double pert		= 0.05;		// must be 0.05 in scenario_predict.
	TESTCASE_OK("insert_and_update", insert_update_test, projectname, trials, instances, func, start_seed, pert, 9877);

	// insert and update.
	func = 2;
	TESTCASE_OK("insert_and_update", insert_update_test, projectname, trials, instances, func, start_seed, pert, 14391);

	// rebuild.
	TESTCASE_OK("rebuild", rebuld_test, projectname, 14435);

	// insert and update.
	func = 3;
	TESTCASE_OK("insert_and_update", insert_update_test, projectname, trials, instances, func, start_seed, pert, 19239);

	// rebuild.
	TESTCASE_OK("rebuild", rebuld_test, projectname, 22066);

	// predict test.
	TESTCASE_OK("predict", predict_test, projectname, instances, std::vector<int>{{1, 2, 3}});

	// insert and update.
	func = 4;
	TESTCASE_OK("insert_and_update", insert_update_test, projectname, trials, instances, func, start_seed, pert, 28643);

	// predict test2.
	TESTCASE_OK("predict", predict_test2, projectname, instances, 4);
}

inline void scenario_limit_chunk(_in const std::string& projectname) {
	// create project.
	TESTCASE_OK("create_project", create_project_test, projectname);

	// test parameter.
	int trials		= 1;
	int instances	= 100;
	int func		= 1;
	int seed		= 0;
	double pert		= 0.05;

	// build 5 chunks. (5 * 100 = 500 instances)

	func = 1;
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, seed++, pert, 99);

	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, seed++, pert, 148);

	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, seed++, pert, 202);

	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, seed++, pert, 249);

	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, seed++, pert, 294);

	// rebuild.
	TESTCASE_OK("rebuild", rebuld_test, projectname, 480);

	// add 2 chunks. (7 * 100 = 700 instances)
	func = 1;
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, seed++, pert, 530);

	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, seed++, pert, 618);

	// limit.chunk on. (limit 300 ~ 600)
	TESTCASE_OK("limit_chunk_property_on", set_property_test, projectname, "limit.chunk.instance_upper_bound", "600");
	TESTCASE_OK("limit_chunk_property_on", set_property_test, projectname, "limit.chunk.instance_lower_bound", "300");
	TESTCASE_OK("limit_chunk_property_on", set_property_test, projectname, "limit.chunk.use", "true");

	// check global variable.
	TESTCASE_OK("global_variable_test", global_variable_test_int64, projectname, "instance_count", 700);

	// add one chunk.
	func = 2;
	TESTCASE_OK("concept_drifted", insert_update_test, projectname, trials, instances, func, seed++, pert, 226);

	// check global variable.
	TESTCASE_OK("global_variable_test", global_variable_test_int64, projectname, "instance_count", 300);

	// add 5 chunks.
	func = 1;
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, seed++, pert, 279);
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, seed++, pert, 329);
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, seed++, pert, 376);
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, seed++, pert, 158);
	TESTCASE_OK("no_concept_drifting", insert_update_test, projectname, trials, instances, func, seed++, pert, 213);

	// check global variable.
	TESTCASE_OK("global_variable_test", global_variable_test_int64, projectname, "instance_count", 400);
	TESTCASE_OK("global_variable_test", global_variable_test_int64, projectname, "instance_correct_count", 213);

	// rebuild.
	TESTCASE_OK("rebuild", rebuld_test, projectname, 355);
	TESTCASE_OK("global_variable_test", global_variable_test_int64, projectname, "instance_correct_count", 355);
}

#endif // HEADER_TESTSCENARIO_HPP
