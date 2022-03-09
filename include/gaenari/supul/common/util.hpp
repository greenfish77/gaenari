#ifndef HEADER_GAENARI_SUPUL_COMMON_UTIL_HPP
#define HEADER_GAENARI_SUPUL_COMMON_UTIL_HPP

namespace supul {
namespace common {

inline void get_paths(_in const std::string& base_dir, _out type::paths& paths) {
	// clear
	paths = type::paths();

	// directories.

	// base_dir.
	paths.base_dir = std::filesystem::absolute(base_dir).string();
	std::filesystem::create_directories(paths.base_dir);

	// conf_dir.
	paths.conf_dir = common::path_join_const(paths.base_dir, "conf");
	std::filesystem::create_directories(paths.conf_dir);

	// sqlite_dir.
	paths.sqlite_dir = common::path_join_const(paths.base_dir, "sqlite");
	std::filesystem::create_directories(paths.sqlite_dir);

	// set file paths.

	// <base_dir>/property.txt
	paths.property_txt = common::path_join_const(paths.base_dir, "property.txt");

	// <base_dir>/conf/attributes.json
	paths.attributes_json = common::path_join_const(paths.conf_dir, "attributes.json");
}

inline void create_empty_attributes_json_if_not_existed(_in const std::string& json_path) {
	// exit if exsited.
	if (std::filesystem::exists(json_path)) return;

	std::string json =
		"{"					"\n"
		"  'revision': 0,"	"\n"
		"  'fields': {"		"\n"
		"  },"				"\n"
		"  'x': [],"		"\n"
		"  'y': null"		"\n"
		"}";

	// replace ' -> "
	gaenari::common::string_replace(json, "\'", "\"");

	// save to file.
	gaenari::common::save_to_file(json_path, json);
}

// alpha, number, _, -, .
// ', " characters will not be added in the future.
inline bool good_char(_in const char c) {
	if (std::isalnum(c)) return true;
	if (c == '_') return true;
	if (c == '-') return true;
	if (c == '.') return true;
	return false;
}

// alpha, number, _, -, .
inline bool good_name(_in const std::string& name) {
	size_t i = 0;
	size_t len = name.length();
	const char* s = name.c_str();
	for (i = 0; i < len; i++) {
		if (not good_char(s[i])) return false;
	}
	return true;
}

// good_name and have a dot(.).
inline bool good_name_with_dot(_in const std::string& name) {
	size_t i = 0;
	size_t len = name.length();
	int dot_count = 0;
	const char* s = name.c_str();
	for (i = 0; i < len; i++) {
		if (not good_char(s[i])) return false;
		if (s[i] == '.') {
			dot_count++;
			if (dot_count >= 2) return false;
		}
	}
	if (dot_count != 1) return false;
	return true;
}

// good name or have a dot(.).
inline bool good_name_or_dot(_in const std::string& name) {
	size_t i = 0;
	size_t len = name.length();
	int dot_count = 0;
	const char* s = name.c_str();
	for (i = 0; i < len; i++) {
		if (not good_char(s[i])) return false;
		if (s[i] == '.') {
			dot_count++;
			if (dot_count >= 2) return false;
		}
	}
	return true;
}

inline void read_attributes(_in const std::string& json_path, _out type::attributes& attributes) {
	std::set<std::string> dup;

	// clear.
	attributes.clear();
	
	// read json and parse.
	std::string json_text;
	gaenari::common::read_from_file(json_path, json_text);
	gaenari::common::json_insert_order_map json;
	json.parse(json_text.c_str());
	json_text.clear(); json_text.shrink_to_fit();

	// get attributes.json values.
	std::string empty;
	auto fields_names = json.object_names({"fields"});
	auto x_count = json.array_size({"x"});
	attributes.revision = json.get_value_numeric({"revision"}, -1);
	attributes.y = json.get_value({"y"}, empty);

	// check empty y.
	if (attributes.y.empty()) THROW_SUPUL_ERROR("missing y in attributes.json.");

	// first, add id.
	attributes.fields["id"] = type::field_type::BIGINT;

	// get fields.
	for (auto& name: fields_names) {
		// `id' is not allowed in the json field. `id` is implicitly added.
		// only alphabets, numbers, and _ are allowed for name.
		if (gaenari::common::strcmp_ignore_case(name, "id")) THROW_SUPUL_ERROR("invalid field name: " + name.get());
		if (not good_name(name)) THROW_SUPUL_ERROR("invalid field name: " + name.get());
		
		// check duplicated name.
		auto lower = gaenari::common::str_lower_const(name.get());
		if (dup.count(lower) == 1) THROW_SUPUL_ERROR("the name is already existed: " + name.get());
		dup.insert(lower);

		// set map (name, type).
		auto& v = json.get_value({"fields", name}, empty);
		if (gaenari::common::strcmp_ignore_case(v, "REAL")) {
			attributes.fields[name] = type::field_type::REAL;
		} else if (gaenari::common::strcmp_ignore_case(v, "INTEGER")) {
			attributes.fields[name] = type::field_type::INTEGER;
		} else if (gaenari::common::strcmp_ignore_case(v, "TEXT_ID")) {
			attributes.fields[name] = type::field_type::TEXT_ID;
		} else if (gaenari::common::strcmp_ignore_case(v, "TEXT")) {
			attributes.fields[name] = type::field_type::TEXT;
		} else if (gaenari::common::strcmp_ignore_case(v, "BIGINT")) {
			attributes.fields[name] = type::field_type::BIGINT;
		} else if (gaenari::common::strcmp_ignore_case(v, "SMALLINT")) {
			attributes.fields[name] = type::field_type::SMALLINT;
		} else THROW_SUPUL_ERROR("invalid field type: " + v);
	}

	// get x.
	attributes.x.resize(x_count);
	for (size_t i=0; i<x_count; i++) {
		attributes.x[i] = json.get_value({"x", i}, empty);
		if (attributes.x[i].empty()) THROW_SUPUL_ERROR("invalid x value(empty).");
		if (attributes.fields.end() == attributes.fields.find(attributes.x[i])) THROW_SUPUL_ERROR("not found x in fields: " + attributes.x[i]);
	}

	// check y in x.
	if (attributes.x.end() != std::find(attributes.x.begin(), attributes.x.end(), attributes.y)) THROW_SUPUL_ERROR("y in x.");
	
	// check y in fields.
	if (attributes.fields.end() == attributes.fields.find(attributes.y)) THROW_SUPUL_ERROR(attributes.y + " is not found in attributes.json.");

	if (attributes.revision != 0) THROW_SUPUL_ERROR("changing fields is not yet supported.");
}

inline void validate_csv_heaer_with_fields(_in const std::vector<std::string>& header, _in const type::fields& fields) {
	// check that all fields are included in the header.
	for (auto& field: fields) {
		auto& field_name = field.first;
		auto& field_type = field.second;
		if (field_name == "id") continue;
		if (header.end() == std::find(header.begin(), header.end(), field_name)) THROW_SUPUL_ERROR("not found header name: " + field_name);
	}
}

// AAA         -> "AAA"
// AAA.BBB     -> "AAA"."BBB"
// AAA.BBB.CCC -> error
// for performance, returns a reference.
// caller passes a temporary `_`.
inline std::string& quote(_in const std::string& name, _in _out std::string& _) {
	const char* s = name.c_str();
	_.reserve(name.length() + 2);
	bool dot = false;
	_ = "\"";
	for (;;s++) {
		if (*s == '\0') break;
		if (*s == '.') {
			if (dot) THROW_SUPUL_INTERNAL_ERROR1("already dot processed.");
			_ += "\".\"";
			dot = true;
			continue;
		}
		_.push_back(*s);
	}
	_ += "\"";
	return _;
}

inline std::string quote(_in const std::string& name) {
	std::string _;
	return quote(name, _);
}

// check validation of property.txt.
inline bool check_valid_property(_in const gaenari::common::prop& prop) {
	auto prefix = prop.get("db.tablename.prefix", "");
	if (not prefix.empty()) {
		if (not common::good_name(prefix)) return false;
	}

	return true;
}

// get value from map_t<std::string, type::variant>.
template <typename map_t = type::map_variant, typename T>
inline void get_variant(_in const map_t& m, _in const std::string& name, _out T& v, _option_in bool strict_type_check = true) {
	auto find = m.find(name);
	if (find == m.end()) THROW_SUPUL_INTERNAL_ERROR0;
	auto& var = find->second;
	auto index = var.index();

	if constexpr (std::is_same<int64_t, T>::value) {
		if (index == 1) { v = std::get<1>(var); return; }						// int64	-> int64
		if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
		if (index == 2) { v = static_cast<int64_t>(std::get<2>(var)); return; } // double	-> int64
	} else if constexpr (std::is_same<int, T>::value) {
		if (index == 1) { v = static_cast<int>(std::get<1>(var)); return; }		// int64	-> int
		if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
		if (index == 2) { v = static_cast<int>(std::get<2>(var)); return; }		// double	-> int
	} else if constexpr (std::is_same<double, T>::value) {
		if (index == 2) { v = std::get<2>(var); return; }						// double	-> double
		if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
		if (index == 1) { v = static_cast<double>(std::get<1>(var)); return; }	// int64	-> double
	} else {
		static_assert(gaenari::type::dependent_false_v<T>, "internal compile error!");
	}
	THROW_SUPUL_INTERNAL_ERROR0;
}

// get value from map_t<std::string, type::variant>.
template <typename map_t = type::map_variant>
inline int get_variant_int(_in const map_t& m, _in const std::string& name, _option_in bool strict_type_check = true) {
	auto find = m.find(name);
	if (find == m.end()) THROW_SUPUL_ERROR("name not found: " + name);
	auto& var = find->second;
	auto index = var.index();
	if (index == 1) return static_cast<int>(std::get<1>(var));					// int64_t.
	if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
	if (index == 2) return static_cast<int>(std::get<2>(var));					// double.
	if (index == 3) return std::stoi(std::get<3>(var));							// string.
	if (index == 0) return 0;													// monostate.
	THROW_SUPUL_INTERNAL_ERROR0;
	return 0;
}

// get value from map_t<std::string, type::variant>.
template <typename map_t = type::map_variant>
inline int64_t get_variant_int64(_in const map_t& m, _in const std::string& name, _option_in bool strict_type_check = true) {
	auto find = m.find(name);
	if (find == m.end()) THROW_SUPUL_ERROR("name not found: " + name);
	auto& var = find->second;
	auto index = var.index();
	if (index == 1) return std::get<1>(var);									// int64_t.
	if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
	if (index == 2) return static_cast<int64_t>(std::get<2>(var));				// double.
	if (index == 3) return static_cast<int64_t>(std::stoll(std::get<3>(var)));	// string.
	if (index == 0) return 0;													// monostate.
	THROW_SUPUL_INTERNAL_ERROR0;
	return 0;
}

// get value from map_t<std::string, type::variant>.
template <typename map_t = type::map_variant>
inline size_t get_variant_size(_in const map_t& m, _in const std::string& name, _option_in bool strict_type_check = true) {
	auto find = m.find(name);
	if (find == m.end()) THROW_SUPUL_ERROR("name not found: " + name);
	auto& var = find->second;
	auto index = var.index();
	if (index == 1) return static_cast<size_t>(std::get<1>(var));				// int64_t.
	if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
	if (index == 2) return static_cast<size_t>(std::get<2>(var));				// double.
	if (index == 3) return static_cast<size_t>(std::stoll(std::get<3>(var)));	// string.
	if (index == 0) return 0;													// monostate.
	THROW_SUPUL_INTERNAL_ERROR0;
}

// get value from map_t<std::string, type::variant>.
template <typename map_t = type::map_variant>
inline double get_variant_double(_in const map_t& m, _in const std::string& name, _option_in bool strict_type_check = true) {
	auto find = m.find(name);
	if (find == m.end()) THROW_SUPUL_ERROR("name not found: " + name);
	auto& var = find->second;
	auto index = var.index();
	if (index == 2) return std::get<2>(var);									// double.
	if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
	if (index == 1) return static_cast<double>(std::get<1>(var));				// int64_t.
	if (index == 3) return std::stod(std::get<3>(var));							// string.
	if (index == 0) return 0.0;													// monostate.
	THROW_SUPUL_INTERNAL_ERROR0;
	return 0;
}

// get value from map_t<std::string, type::variant>.
template <typename map_t = type::map_variant>
inline std::string get_variant_string(_in const map_t& m, _in const std::string& name, _option_in bool strict_type_check = true) {
	auto find = m.find(name);
	if (find == m.end()) THROW_SUPUL_ERROR("name not found: " + name);
	auto& var = find->second;
	auto index = var.index();
	if (index == 3) return std::get<3>(var);									// string.
	if (strict_type_check) THROW_SUPUL_ERROR("type check error: " + name);
	if (index == 1) return std::to_string(std::get<1>(var));					// int64_t.
	if (index == 2) return gaenari::common::dbl_to_string(std::get<2>(var));	// double.
	if (index == 0) return "";													// monostate.
	THROW_SUPUL_INTERNAL_ERROR0;
	return 0;
}

// get value from map_t<std::string, type::variant>.
// run on strict type check.
template <typename map_t = type::map_variant>
inline const std::string& get_variant_string_ref(_in const map_t& m, _in const std::string& name) {
	auto find = m.find(name);
	if (find == m.end()) THROW_SUPUL_ERROR("name not found: " + name);
	auto& var = find->second;
	auto index = var.index();
	if (index == 3) return std::get<3>(var);
	THROW_SUPUL_INTERNAL_ERROR0;
}

// returns pair of ({f1, f2, f3}, {INTEGER, TEXT, TEXT}).
inline auto fields_to_vector(_in const type::fields& fields, _in const std::vector<std::string>& exclude_names) -> std::pair<std::vector<std::reference_wrapper<const std::string>>, std::vector<type::field_type>>{
	std::pair<std::vector<std::reference_wrapper<const std::string>>, std::vector<type::field_type>> ret;

	for (const auto& it: fields) {
		const auto& name = it.first;
		const auto& type = it.second;
		if (std::find(exclude_names.begin(), exclude_names.end(), name) != exclude_names.end()) {
			// exclude.
			continue;
		}
		ret.first.emplace_back(name);
		ret.second.emplace_back(type);
	}

	return ret;
}

// convert to fields to string `"f1","f2","f3"`.
// force_question on, returns `?,?,?`. (in this case, table_name and add_as are not affected.)
// force_question off,
//     table_name "t1",				returns `t1."f1",t1."f2",t1."f3"`. (remark: table name is not quoted.)
//          add_as on,				returns `t1."f1" as "f1",t1."f2" AS "f2",t1."f3" as "f3"`
//				as_table_name "T",	returns `t1."f1" as "T.f1",t1."f2" AS "T.f2",t1."f3" as "T.f3"`
//     empty table_name,			returns `"f1","f2","f3"`
//          add_on on,				returns `"f1","f2","f3"` (not affected.)
inline std::string get_names(_in const type::fields& fields, _in const std::vector<std::string>& exclude_names, _option_in bool force_question, _option_in const std::string& table_name, _option_in bool add_as, _option_in const std::string& as_table_name) {
	std::string ret;
	if (fields.empty()) return "";

	// get names and set text.
	auto [names, _] = fields_to_vector(fields, exclude_names);
	for (const auto& _name: names) {
		const auto& name = _name.get();
		if (force_question) {
			ret += "?,";
		} else {
			if (table_name.empty()) {
				ret += common::quote(name) + ',';
			} else {
				if (add_as) {
					if (as_table_name.empty()) {
						ret += table_name + '.' + common::quote(name) + " AS " + common::quote(name) + ',';
					} else {
						ret += table_name + '.' + common::quote(name) + " AS \"" + as_table_name + '.' + name + "\",";
					}
				} else {
					ret += table_name + '.' + common::quote(name) + ',';
				}
			}
		}
	}
	ret.pop_back();
	return ret;
}

// add the get_names() result.
inline std::string& get_names_append(_in _out std::string& names, _in const type::fields& fields, _in const std::vector<std::string>& exclude_names, _option_in bool force_question, _option_in const std::string& table_name, _option_in bool add_as, _option_in const std::string& as_table_name) {
	if (not names.empty()) names += ',';
	std::string n = get_names(fields, exclude_names, force_question, table_name, add_as, as_table_name);
	names += std::move(n);
	return names;
}

template<typename map = type::map_variant>
inline int64_t get_generic_map_value_int64(_in const map& x, _in const std::string& name) {
	// map type property.
	using K = typename map::key_type;
	using T = typename map::mapped_type;

	// type checking...
	// key type must be std::string.
	if constexpr (not std::is_same<std::string, K>::value) static_assert(gaenari::type::dependent_false_v<T>, "internal compile error!");

	if constexpr (std::is_same<type::value_variant, T>::value) {
		return get_variant_int64(x, name, true);
	} else if constexpr (std::is_same<std::string, T>::value) {
		auto find = x.find(name);
		if (find == x.end()) THROW_SUPUL_ERROR("name not found: " + name);
		return static_cast<int64_t>(std::stoll(find->second));
	} else {
		static_assert(gaenari::type::dependent_false_v<T>, "internal compile error!");
	}
}

// gaenari::common::f(...) => common::f(...)
// format message.
// r = f("[%0][%1][%0][%%1][%9]", {1, "hello"}) => "[1][hello][1][%1][%9]"
// inline std::string f(_in const std::string& fmt, _in const std::vector<std::variant<std::monostate,int,int64_t,double,std::string>>& params) {
auto& f = gaenari::common::f;

template <typename map_t = std::map<std::string, std::string>>
void to_map_variant(_in const map_t& i, _out type::map_variant& o, _in const type::fields& fields, _in const gaenari::common::string_table& string_table) {
	int id = 0;

	if (i.empty()) THROW_SUPUL_ERROR("empty data.");

	// clear output.
	o.clear();

	for (const auto& it: i) {
		const auto& name  = it.first;
		const auto& value = it.second;

		auto find = fields.find(name);
		if (find == fields.end()) continue;
		const auto& type = find->second;

		switch (type) {
		case type::field_type::INTEGER:
		case type::field_type::BIGINT:
		case type::field_type::SMALLINT:
		case type::field_type::TINYINT:
			o[name] = static_cast<int64_t>(std::stoll(value));
			break;
		case type::field_type::REAL:
			o[name] = std::stod(value);
			break;
		case type::field_type::TEXT_ID:
			id = string_table.get_id(value);
			if (id < 0) THROW_SUPUL_ERROR1("not found in string table: %0.", value);
			o[name] = static_cast<int64_t>(id);
			break;
		default:
			THROW_SUPUL_INTERNAL_ERROR0;
		}
	}
}

auto get_confusion_matrix_diff(_in const std::unordered_map<int, std::unordered_map<int, int64_t>>& before, _in const std::unordered_map<int, std::unordered_map<int, int64_t>>& after) -> std::unordered_map<int, std::unordered_map<int, int64_t>> {
	std::unordered_map<int, std::unordered_map<int, int64_t>> ret;

	ret = before;
	for (const auto& it: after) {
		auto& actual = it.first;
		for (const auto& it2: it.second) {
			auto& predicted = it2.first;
			auto& after_count = it2.second;
			ret[actual][predicted] = after_count - ret[actual][predicted];
		}
	}

	return ret;
}

inline void verify_confusion_matrix(_in const std::vector<type::map_variant>& a, _in const std::unordered_map<int, std::unordered_map<int, int64_t>>& b) {
	if (a.empty() and b.empty()) return;
	std::set<std::string> check_dup;

	// size check.
	size_t b_size = 0;
	for (auto& it: b) b_size += it.second.size();
	if (a.size() != b_size) THROW_SUPUL_ERROR2("fail to verify_confusion_matrix(1), a.size(%0) != b_size(%1).", a.size(), b_size);

	// check.
	auto b_copied = b;
	for (auto& it: a) {
		auto actual    = common::get_variant_int(it, "actual");
		auto predicted = common::get_variant_int(it, "predicted");
		auto count     = common::get_variant_int64(it, "count");
		if (b_copied.find(actual) == b_copied.end()) THROW_SUPUL_ERROR1("fail to verify_confusion_matrix(2), actual(%0).", actual);
		if (b_copied[actual].find(predicted) == b_copied[actual].end()) THROW_SUPUL_ERROR2("fail to verify_confusion_matrix(3), acutal(%0), predicted(%0).", actual, predicted);
		if (b_copied[actual][predicted] != count) THROW_SUPUL_ERROR4("fail to verify_confusion_matrix(4), b_copied[%0][%1](%2) != count(%3).", actual, predicted, b_copied[actual][predicted], count);

		// check duplicated.
		auto c = F("%0_%1", {actual, predicted});
		if (check_dup.count(c) == 1) THROW_SUPUL_ERROR2("fail to verify_confusion_matrix(5), actual(%0), predicted(%0).", actual, predicted);
		check_dup.insert(c);
	}

	// passed.
}

} // common
} // supul

#endif // HEADER_GAENARI_SUPUL_COMMON_UTIL_HPP
