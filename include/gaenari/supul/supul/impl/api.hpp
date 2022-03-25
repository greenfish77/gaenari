#ifndef HEADER_GAENARI_SUPUL_SUPUL_IMPL_API_HPP
#define HEADER_GAENARI_SUPUL_SUPUL_IMPL_API_HPP

// footer included from supul.hpp
namespace supul {
namespace supul {

// create a supul empty project.
// static function. just call supul::supul::supul_t::api::project::create(...).
inline bool supul_t::api::project::create(_in const std::string& base_dir) noexcept {
	common::function_logger l{__func__, "project"};
	try {
		supul::supul_t::create_project(base_dir);
		return true;
	} catch(...) {
		return false;
	}
}

// set project property.
// static function. just call supul::supul::supul_t::api::project::set_property(...).
inline bool supul_t::api::project::set_property(_in const std::string& base_dir, _in const std::string& name, _in const std::string& value) noexcept {
	try {
		auto path = common::path_join_const(base_dir, "property.txt");
		gaenari::common::prop property;
		if ((not property.read(path, true)) or (property.empty())) {
			gaenari::logger::error(F("fail to read %0.", {path}));
			return false;
		}
		property.set(name, value);
		if (not property.save()) {
			gaenari::logger::error(F("fail to write %0.", {path}));
			return false;
		}
		return true;
	} catch(...) {
		gaenari::logger::error("fail to project::set_property.");
		return false;
	}
}

// add a field to attributes.json.
// static function. just call supul::supul::supul_t::api::project::add_field(...).
inline bool supul_t::api::project::add_field(_in const std::string& base_dir, _in const std::string& name, _in const std::string& dtype) noexcept {
	// choose REAL(numeric), INTEGER(numeric), and TEXT_ID(nominal).
	if ((dtype != "REAL") and (dtype != "INTEGER") and (dtype != "TEXT_ID")) {
		gaenari::logger::error(F("invalid dtype: %0.", {dtype}));
		return false;
	}

	try {
		auto json_path = common::path_joins_const(base_dir, {"conf", "attributes.json"});
		using json_t = gaenari::common::json_insert_order_map;
		json_t json;
		gaenari::logger::info("project::add_field add({0}, {1}) to attributes.json.", {name, dtype});
		std::string json_text;
		gaenari::common::read_from_file(json_path, json_text);
		json.parse(json_text.c_str());
		json.set_value({"fields", name}, dtype);
		json_text = json.to_string();
		gaenari::common::save_to_file(json_path, json_text);
		return true;
	} catch(...) {
		gaenari::logger::error("fail to project::add_field.");
		return false;
	}
}

// set x names.
// static function. just call supul::supul::supul_t::api::project::x(...).
inline bool supul_t::api::project::x(_in const std::string& base_dir, _in const std::vector<std::string>& x) noexcept {
	try {
		auto json_path = common::path_joins_const(base_dir, {"conf", "attributes.json"});
		using json_t = gaenari::common::json_insert_order_map;
		json_t json;

		gaenari::logger::info("project::x {0}.", {gaenari::common::vec_to_string(x)});
		std::string json_text;
		gaenari::common::read_from_file(json_path, json_text);
		json.parse(json_text.c_str());
		for (const auto& i : x) json.set_value({"x", json_t::path_option::array_append}, i);
		json_text = json.to_string();
		gaenari::common::save_to_file(json_path, json_text);
		return true;
	} catch(...) {
		gaenari::logger::error("fail to project::x.");
		return false;
	}
}

// set y name.
// static function. just call supul::supul::supul_t::api::project::y(...).
inline bool supul_t::api::project::y(_in const std::string& base_dir, _in const std::string& y) noexcept {
	try {
		auto json_path = common::path_joins_const(base_dir, {"conf", "attributes.json"});
		using json_t = gaenari::common::json_insert_order_map;
		json_t json;

		gaenari::logger::info("project::y {0}.", {y});
		std::string json_text;
		gaenari::common::read_from_file(json_path, json_text);
		json.parse(json_text.c_str());
		json.set_value({"y"}, y);
		json_text = json.to_string();
		gaenari::common::save_to_file(json_path, json_text);
		return true;
	}
	catch (...) {
		gaenari::logger::error("fail to project::y.");
		return false;
	}
}

// initialize supul.
inline bool supul_t::api::lifetime::open(_in const std::string& base_dir) noexcept {
	common::function_logger l{__func__, "lifetime"};
	try {
		api.supul.init(base_dir);
		api.initialized = true;
		return true;
	} catch(...) {
		l.failed();
		api.errormsg = exceptions::catch_all();
		return false;
	}
}

// deinitialize supul.
// no need for explicit calls.
inline void supul_t::api::lifetime::close(void) noexcept {
	if (not api.initialized) return;
	common::function_logger l{__func__, "lifetime"};
	try {
		api.supul.deinit();
		api.initialized = false;
	} catch(...) {
		l.failed();
		api.errormsg = exceptions::catch_all();
	}
}

// insert instances into databases in chunk.
// insert only, no follow-up operation.
// it takes as long as the size of the chunk.
inline bool supul_t::api::model::insert_chunk_csv(_in const std::string& csv_file_path) noexcept {
	common::function_logger l{__func__, "model"};
	try {
		api.supul.model.insert_chunk_csv(csv_file_path);
	} catch(...) {
		l.failed();
		api.errormsg = exceptions::catch_all();
		return false;
	}
	return true;
}

// all inserted instances must be evaluated.
// evalues unevaluated instances and updates instance information.
// if the tree does not exist, it trains the tree with all instances.
// it takes as long as the size of the current model and the number of unevaluated instances.
inline bool supul_t::api::model::update(void) noexcept {
	common::function_logger l{__func__, "model"};
	try {
		api.supul.model.update();
		return true;
	} catch(...) {
		l.failed();
		api.errormsg = exceptions::catch_all();
		return false;
	}
}

// evaluated instances can become less accurate over time.
// - it finds weak instances with low accuracy,
// - re-training them,
// - and combines them with existing tree.
// therefore, the decrease in accuracy can be prevented as much as possible.
// it takes as long as the total and weak number of instances.
inline bool supul_t::api::model::rebuild(void) noexcept {
	common::function_logger l{__func__, "model"};
	try {
		api.supul.model.rebuild();
		return true;
	} catch(...) {
		l.failed();
		api.errormsg = exceptions::catch_all();
		return false;
	}
}

// predict with (name, value) map.
// the value of map is a string, which is automatically converted to a value according to attributes.json.
// that is, for ("feature1", "3.14"), "3.14" is automatically converted to 3.14.
inline auto supul_t::api::model::predict(_in const std::unordered_map<std::string, std::string>& x) noexcept -> type::predict_result {
	common::function_logger l{__func__, "model", common::function_logger::show_option::error_only};

	// set default return values.
	type::predict_result ret;

	try {
		// convert map(string, string) -> map(string, variant).
		type::map_variant dtyped_x;
		common::to_map_variant(x, dtyped_x, api.supul.schema.get_table_info(type::table::instance).fields, api.supul.string_table.get_table());

		// predict.
		// use const to make the result immutable.
		const type::treenode_db p = api.supul.model.predict(dtyped_x);

		// label_id(int) to label(string).
		auto& label = api.supul.string_table.get_table().get_string(p.leaf_info.label_index);

		// set ret.
		ret.label_index = p.leaf_info.label_index;
		ret.label = label;
		ret.correct_count = p.leaf_info.correct_count;
		ret.total_count = p.leaf_info.total_count;
		ret.accuracy = p.leaf_info.accuracy;

		return ret;
	} catch(...) {
		ret.clear();
		ret.error = true;
		l.failed();
		api.errormsg = exceptions::catch_all();
		ret.errormsg = api.errormsg;
		return ret;
	}
}

// returns the report as a json string.
// option format:
// {
//   "categories":["...",...],
// }
// - categories: specifies the report category. default(all).
inline auto supul_t::api::report::json(_in const std::string& option_as_json) noexcept -> std::string {
	common::function_logger l{__func__, "report"};
	try {
		supul_t::report report{api.supul};
		return report.json(option_as_json);
	} catch(...) {
		api.errormsg = exceptions::catch_all();
		auto _errormsg = gaenari::common::json::static_escape<true>(api.errormsg);
		std::string errormsg = "N/A";
		if (_errormsg) errormsg = _errormsg.value();
		return F("{\"error\":true,\"errormsg\":\"%0\"}", {errormsg});
	}
}

// returns a script for drawing gnuplot charts with a json report.
// static function. just call supul::supul::supul_t::api::report::gnuplot(...).
// (requires gnuplot 5.x to be installed.)
//
// option)
// - [req] terminal: "pngcairo", "wxt", ... (http://gnuplot.info/docs_5.5/Terminals.html)
// - [opt] terminal_option: "`Times-New-Roman,10` size 800,800" (automatically ` to ")
// - [opt] output_filepath: file path to save if type is png.
// - [opt] plt_filepath: file path to gnuplot script. you need to define a `set terminal` for gnuplot.
//
// ex)
// - {{"terminal", "pngcairo"}, {"output_filepath", "/tmp/test.png"}, {"plt_filepath", "/tmp/test.plt"}}
// - {{"terminal", "pngcairo"}, {"terminal_option", "font `Times-New-Roman,10` size 800,800"}, {"output_filepath", "/tmp/test.png"}}
// - {{"terminal", "wxt"}}
// - {{"terminal", "wxt"}, {"terminal_option", "font `Times-New-Roman,10` size 800,800"}}
// - {{"terminal", "wxt"}, {"plt_filepath", "/tmp/plot.plt"}}
inline auto supul_t::api::report::gnuplot(_in const std::string& json, _in const std::map<std::string, std::string>& options) noexcept -> std::optional<std::string> {
	common::function_logger l{__func__, "report"};
	std::string ret;
	try {
		return supul_t::report::gnuplot(json, options);
	} catch(...) {
		return std::nullopt;
	}
}

// return gaenari version.
inline auto supul_t::api::misc::version(void) noexcept -> const std::string& {
	return gaenari::common::version();
}

// returns the reason text for the error in the previous function call.
// (static functions are not supported.)
inline auto supul_t::api::misc::errmsg(void) const noexcept -> std::string {
	return api.errormsg;
}

// set property.
// it is  function to store in cache memory.
// call the save() to save configuration file.
// do not write function logger.
inline bool supul_t::api::property::set_property(_in const std::string& name, _in const std::string& value) noexcept {
	try {
		api.supul.prop.set(name, value);
		return true;
	} catch(...) {
		api.errormsg = exceptions::catch_all();
		gaenari::logger::error(api.errormsg);
		return false;
	}
}

// get property.
// do not write function logger.
// throw no exceptions on error. just return default.
inline auto supul_t::api::property::get_property(_in const std::string& name, _in const std::string& def) noexcept -> std::string {
	try {
		return api.supul.prop.get(name, def);
	} catch(...) {
		api.errormsg = exceptions::catch_all();
		gaenari::logger::error(api.errormsg);
		return def;
	}
}

// save set_property() to a configuration file.
inline bool supul_t::api::property::save(void) noexcept {
	try {
		api.supul.prop.save();
		return true;
	} catch(...) {
		api.errormsg = exceptions::catch_all();
		gaenari::logger::error(api.errormsg);
		return false;
	}
}

// reload configuration file.
inline bool supul_t::api::property::reload(void) noexcept {
	try {
		api.supul.prop.reload();
		return true;
	} catch(...) {
		api.errormsg = exceptions::catch_all();
		gaenari::logger::error(api.errormsg);
		return false;
	}
}

// cache database contents to improve performance.
// make sure the cache and database contents match.
// used for testing and not normally called.
inline bool supul_t::api::test::verify(void) noexcept {
	common::function_logger l{__func__, "test"};
	try {
		api.supul.model.verify_all();
		return true;
	} catch(...) {
		l.failed();
		api.errormsg = exceptions::catch_all();
		return false;
	}
}

} // supul
} // supul

#endif // HEADER_GAENARI_SUPUL_SUPUL_IMPL_API_HPP
