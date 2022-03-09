#ifndef HEADER_GAENARI_SUPUL_DB_SCHEMA_HPP
#define HEADER_GAENARI_SUPUL_DB_SCHEMA_HPP

namespace supul {
namespace db {

class schema {
public:
	schema()  = default;
	~schema() = default;

public:
	void set(_in const type::attributes& attributes, _in const std::string& tablename_prefix);
	int64_t version(void) const;
	std::string get_sql(_in const std::string& sql) const;
	const type::table_info& get_table_info(_in type::table t) const;
	const std::vector<type::table_info>& get_tables(void) const;
	const type::field_type field_type(_in type::table t, _in const std::string& name) const;
	const type::fields fields_include(_in type::table t, _option_in const std::vector<std::string>& field_names) const;
	const type::fields fields_join(_in const std::vector<std::pair<type::table, std::vector<std::string>>>& table_field_names, _in bool exclude) const;
	const type::fields fields_exclude(_in type::table t, _in const std::vector<std::string>& exclude_field_names) const;
	const type::fields fields_merge(_in const std::vector<type::fields> fieldss) const;
	const std::string& get_real_table_name(_in const std::string& table_name) const;
	const std::string& get_real_table_name(_in type::table t) const;

protected:
	int64_t schema_version = -1;
	std::map<std::string, std::string> table_name_map;
	std::vector<type::table_info> tables;
	std::string y;
};

const type::table_info& schema::get_table_info(_in type::table t) const {
	// exceptions can be raised using `at`.
	return tables.at((size_t)t);
}

const std::vector<type::table_info>& schema::get_tables(void) const {
	return tables;
}

// convert sql text.
// select * from ${instance}
// ->
// select * from "instance".			(when prefix name is "")
// select * from "project1_instance".	(when prefix name is "project1")
//
// select "%y%" from ${instance}
// ->
// select "..." from "instance".		("..." is y in attributes.json)
inline std::string schema::get_sql(_in const std::string& sql) const {
	std::string ret = sql;
	for (auto& it: table_name_map) gaenari::common::string_replace(ret, "${" + it.first + '}', common::quote(it.second));
	gaenari::common::string_replace(ret, "%y%", y);
	if (strstr(ret.c_str(), "${")) THROW_SUPUL_ERROR("invalid sql, " + sql);
	return ret;
}

const std::string& schema::get_real_table_name(_in const std::string& table_name) const {
	auto find = table_name_map.find(table_name);
	if (find == table_name_map.end()) THROW_SUPUL_ERROR("table_name is not found, " + table_name);
	return find->second;
}

const std::string& schema::get_real_table_name(_in type::table t) const {
	auto& table = get_table_info(t);
	return get_real_table_name(table.name);
}

const type::field_type schema::field_type(_in type::table t, _in const std::string& name) const {
	auto& table = get_table_info(t);
	auto find = table.fields.find(name);
	if (find == table.fields.end()) THROW_SUPUL_ITEM_NOT_FOUND(name);
	return find->second;
}

// get a subset of fields.
// if field_names is empty, return all.
const type::fields schema::fields_include(_in type::table t, _option_in const std::vector<std::string>& field_names) const {
	type::fields fields;
	auto& table = get_table_info(t);
	if (field_names.empty()) return table.fields;
	for (const auto& name: field_names) {
		auto find = table.fields.find(name);
		if (find == table.fields.end()) THROW_SUPUL_ERROR("field name is not found, " + name);
		fields[name] = find->second;
	}
	return fields;
}

// get a subset of fields.
const type::fields schema::fields_exclude(_in type::table t, _in const std::vector<std::string>& exclude_field_names) const {
	type::fields fields;
	if (exclude_field_names.empty()) THROW_SUPUL_INTERNAL_ERROR0;
	auto& table = get_table_info(t);
	for (const auto& it: table.fields) {
		const auto& name = it.first;
		const auto& type = it.second;
		if (std::find(exclude_field_names.begin(), exclude_field_names.end(), name) != exclude_field_names.end()) {
			// exclude.
			continue;
		}
		fields[name] = type;
	}
	return fields;
}

const type::fields schema::fields_merge(_in const std::vector<type::fields> fieldss) const {
	type::fields ret;

	for (const auto& fields: fieldss) {
		for (const auto& it: fields) {
			if (ret.find(it.first) != ret.end()) THROW_SUPUL_INTERNAL_ERROR0;
			ret[it.first] = it.second;
		}
	}

	return ret;
}

// get a subset of multiple table fields.
// use <tablename>.<fieldname> format.
// - pair first  : table type.
// - pair second : field names. (exclude or not).
// if field_names is empty, return all.
const type::fields schema::fields_join(_in const std::vector<std::pair<type::table, std::vector<std::string>>>& table_field_names, _in bool exclude) const {
	type::fields fields;
	for (const auto& table_field: table_field_names) {
		auto& t = table_field.first;
		auto& table = get_table_info(t);
		auto& field_names = table_field.second;

		if (field_names.empty()) {
			// all selected.
			for (const auto& it: table.fields) {
				const auto& name = it.first;
				const auto& type = it.second;
				fields[table.name + '.' + name] = type;
			}

			continue;
		}

		if (not exclude) {
			// include.
			for (const auto& name: field_names) {
				auto find = table.fields.find(name);
				if (find == table.fields.end()) THROW_SUPUL_INTERNAL_ERROR0;
				fields[table.name + '.' + name] = find->second;
			}
		} else {
			// exclude.
			for (const auto& it: table.fields) {
				const auto& name = it.first;
				const auto& type = it.second;
				if (std::find(field_names.begin(), field_names.end(), name) != field_names.end()) {
					// found. exclude it.
					continue;
				}
				fields[table.name + '.' + name] = type;
			}
		}
	}

	return fields;
}

inline int64_t schema::version(void) const {
	return this->schema_version;
}

inline void schema::set(_in const type::attributes& attributes, _in const std::string& tablename_prefix) {
	tables.clear();
	table_name_map.clear();

	// set version.
	this->schema_version = 1;

	// set y.
	this->y = attributes.y;

	// set prefix.
	std::string prefix;
	if (not tablename_prefix.empty()) {
		if (not ::supul::common::good_name(tablename_prefix)) THROW_SUPUL_ERROR("invalid prefix name, " + tablename_prefix);
		prefix = tablename_prefix + '_';
	}

	// resize.
	tables.resize((size_t)type::table::table_count);

	// instance table(set fields from attributes.json).
	table_name_map["instance"] = prefix + "instance";
	tables[(size_t)type::table::instance] = {
		"instance",
		type::fields { attributes.fields },
		{},	// no index.
	};

	// instance_info table.
	table_name_map["instance_info"] = prefix + "instance_info";
	tables[(size_t)type::table::instance_info] = {
		"instance_info",
		type::fields {
			{"id",						type::field_type::BIGINT},
			{"ref_instance_id",			type::field_type::BIGINT},
			{"ref_chunk_id",			type::field_type::INTEGER},
			{"ref_leaf_treenode_id",	type::field_type::BIGINT},
			{"weak_count",				type::field_type::INTEGER},
			{"correct",					type::field_type::TINYINT},
		},
		{{"ref_instance_id"}, {"ref_chunk_id"}, {"ref_leaf_treenode_id"}},
	};

	// chunk table.
	table_name_map["chunk"] = prefix + "chunk";
	tables[(size_t)type::table::chunk] = {
		"chunk",
		type::fields {
			{"id",						type::field_type::BIGINT},
			{"datetime",				type::field_type::BIGINT},
			{"updated",					type::field_type::TINYINT},
			{"initial_correct_count",	type::field_type::BIGINT},
			{"total_count",				type::field_type::BIGINT},
			{"initial_accuracy",		type::field_type::REAL},
		},
		{{"updated"}},
	};

	// generation table.
	table_name_map["generation"] = prefix + "generation";
	tables[(size_t)type::table::generation] = {
		"generation",
		type::fields {
			{"id",								type::field_type::BIGINT},
			{"datetime",						type::field_type::BIGINT},
			{"root_ref_treenode_id",			type::field_type::BIGINT},
			{"instance_count",					type::field_type::BIGINT},
			{"weak_instance_count",				type::field_type::BIGINT},
			{"weak_instance_ratio",				type::field_type::REAL},
			{"before_weak_instance_accuracy",	type::field_type::REAL},
			{"after_weak_instance_accuracy",	type::field_type::REAL},
			{"before_instance_accuracy",		type::field_type::REAL},
			{"after_instance_accuracy",			type::field_type::REAL},
		},
		{},	// no index.
	};

	// treenode table.
	table_name_map["treenode"] = prefix + "treenode";
	tables[(size_t)type::table::treenode] = {
		"treenode",
		type::fields {
			{"id",						type::field_type::BIGINT},
			{"ref_generation_id",		type::field_type::BIGINT},
			{"ref_parent_treenode_id",	type::field_type::BIGINT},
			{"ref_rule_id",				type::field_type::BIGINT},
			{"ref_leaf_info_id",		type::field_type::BIGINT},
		},
		{{"ref_rule_id"},{"ref_leaf_info_id"},{"ref_parent_treenode_id"},{"ref_generation_id"}}
	};

	// rule table.
	table_name_map["rule"] = prefix + "rule";
	tables[(size_t)type::table::rule] = {
		"rule",
		type::fields {
			{"id",						type::field_type::BIGINT},
			{"feature_index",			type::field_type::SMALLINT},
			{"rule_type",				type::field_type::TINYINT},
			{"value_type",				type::field_type::TINYINT},
			{"value_integer",			type::field_type::BIGINT},
			{"value_real",				type::field_type::REAL},
		},
		{},	// no index.
	};

	// leaf_info table.
	table_name_map["leaf_info"] = prefix + "leaf_info";
	tables[(size_t)type::table::leaf_info] = {
		"leaf_info",
		type::fields {
			{"id",						type::field_type::BIGINT},
			{"label_index",				type::field_type::INTEGER},
			{"type",					type::field_type::TINYINT},
			{"go_to_ref_generation_id",	type::field_type::BIGINT},
			{"correct_count",			type::field_type::BIGINT},
			{"total_count",				type::field_type::BIGINT},
			{"accuracy",				type::field_type::REAL},
		},
		{{"total_count"},{"accuracy"},{"go_to_ref_generation_id"}},
	};

	// string_table table.
	table_name_map["string_table"] = prefix + "string_table";
	tables[(size_t)type::table::string_table] = {
		"string_table",
		type::fields {
			{"id",						type::field_type::INTEGER},
			{"text",					type::field_type::TEXT},
		},
		{},	// no index.
	};

	// global variable table.
	// remark)
	// only one row, not (key, value) map values structure.
	// it's more clear and specify a type for each column.
	// <one row>				<(key,value) structure>
	// ---+------+------+----	---+------+------
	// id | key1 | key2 | ...	id | key  | value
	// ---+------+------+----	---+------+------
	//  1 | 3.14 | 16   | ...	 1 | key1 | 3.14
	// ---+------+------+----	 2 | key2 | 16.0
	//							...| ...  | ...
	table_name_map["global"] = prefix + "global";
	tables[(size_t)type::table::global] = {
		"global",
		type::fields {
			// when item added, check supul::on_first_initialized().
			{"id",						type::field_type::BIGINT},	// id = 1, only one row.
			{"schema_version",			type::field_type::BIGINT},
			{"instance_count",			type::field_type::BIGINT},
			{"updated_instance_count",	type::field_type::BIGINT},
			{"instance_correct_count",	type::field_type::BIGINT},
			{"instance_accuracy",		type::field_type::REAL},
			{"acc_weak_instance_count",	type::field_type::BIGINT},
		},
		{},	// no index.
	};

	// global confusion matrix table.
	table_name_map["global_confusion_matrix"] = prefix + "global_confusion_matrix";
	tables[(size_t)type::table::global_confusion_matrix] = {
		"global_confusion_matrix",
		type::fields {
			{"id",						type::field_type::BIGINT},
			{"actual",					type::field_type::INTEGER},
			{"predicted",				type::field_type::INTEGER},
			{"count",					type::field_type::BIGINT},
		},
		{},	// no index.
	};
}

} // db
} // supul

#endif // HEADER_GAENARI_SUPUL_DB_SCHEMA_HPP
