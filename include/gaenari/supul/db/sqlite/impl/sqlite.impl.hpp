#ifndef HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_IMPL_HPP
#define HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_IMPL_HPP

// footer included from sqlite.hpp

namespace supul {
namespace db {
namespace sqlite {

inline void sqlite_t::init(_in const gaenari::common::prop& property, _in const type::paths& paths) {
	int rc;
	std::string sql;
	std::map<std::string, std::string> table_name_map;

	// called twice?
	if (this->db) THROW_SUPUL_INTERNAL_ERROR0;

	// open db.
	auto& database_name = property.get("db.dbname");
	if (database_name.empty()) THROW_SUPUL_ERROR("invalid db.dbname(empty).");
	if (not common::good_name(database_name)) THROW_SUPUL_ERROR("invalid db.dbname.");

	// sqlite db file path.
	std::string path = common::path_join_const(paths.sqlite_dir, database_name + ".db");

	// sqlite3 open.
	rc = sqlite3_open(path.c_str(), &this->db);
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to sqlite3_open, " + path, error_desc(rc));
	if (not this->db) THROW_SUPUL_INTERNAL_ERROR0;

	// sqlite PRAGMA settings.
	rc = sqlite3_exec(this->db, "pragma journal_mode = WAL;", nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to pragma journal_mode = WAL.", error_desc(rc));

	// create table if not exsited.
	create_table_if_not_existed(schema);

	// create index if not existed.
	create_index_if_not_existed(schema);

	// stmt pool.
	build_stmt_pool();
}

inline void sqlite_t::deinit(void) {
	// clear stmt pool.
	clear_stmt_pool();

	// close db.
	if (this->db) sqlite3_close(this->db);
	this->db = nullptr;
}

inline void sqlite_t::begin(_in bool exclusive) {
	if (not this->db) THROW_SUPUL_INTERNAL_ERROR0;
	int rc = 0;
	if (exclusive) {
		rc = sqlite3_exec(this->db, "begin exclusive", nullptr, nullptr, nullptr);
	} else {
		rc = sqlite3_exec(this->db, "begin", nullptr, nullptr, nullptr);
	}
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to begin transaction.", error_desc(rc));
}

inline void sqlite_t::commit(void) {
	if (not this->db) THROW_SUPUL_INTERNAL_ERROR0;
	auto rc = sqlite3_exec(this->db, "commit", nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to commit transaction.", error_desc(rc));
}

inline void sqlite_t::rollback(void) {
	if (not this->db) THROW_SUPUL_INTERNAL_ERROR0;
	auto rc = sqlite3_exec(this->db, "rollback", nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to rollback transaction.", error_desc(rc));
}

inline void sqlite_t::lock_timeout(_in int msec) {
	if (not this->db) THROW_SUPUL_INTERNAL_ERROR0;
	auto rc = sqlite3_busy_timeout(this->db, msec);
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to busy_timeout.", error_desc(rc));
}

inline void sqlite_t::build_stmt_pool(void) {
	int rc = 0;
	std::string sql;

	// refer to common.hpp::set_schema(...) to complete the sql statements.

	// clear pool.
	clear_stmt_pool();

	// code structure description.
	// sql = get_sql("... SQL ... ${tablename} ...", table_name_map);	==> prefix is applied by using `${tablename}`.
	// stmt_pool.insert({	stmt_type,									==> id for the previous sql text.
	//						stmt_info{get_stmt(sql)},					==> fixed.
	//						fields});									==> query result columns definition.

	// count_string_table.
	sql = schema.get_sql("SELECT COUNT(*) FROM ${string_table}");
	stmt_pool.insert({	stmt::count_string_table,
						stmt_info{get_stmt(sql),
						type::fields{{"COUNT(*)", type::field_type::INTEGER}}}});

	// get_string_table_last_id.
	sql = schema.get_sql("SELECT id FROM ${string_table} WHERE id=(SELECT MAX(id) FROM ${string_table})");
	stmt_pool.insert({	stmt::get_string_table_last_id, 
						stmt_info{get_stmt(sql), 
						schema.fields_include(type::table::string_table, {"id"})}});

	// get_string_table.
	sql = schema.get_sql("SELECT * FROM ${string_table}");
	stmt_pool.insert({	stmt::get_string_table,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::string_table, {})}});

	// add string table.
	sql = schema.get_sql("INSERT INTO ${string_table} (id, text) VALUES (?,?)");
	stmt_pool.insert({	stmt::add_string_table,
						stmt_info{get_stmt(sql),
						{}}});

	// add chunk.
	sql = schema.get_sql("INSERT INTO ${chunk} (datetime, updated) VALUES (?, 0) RETURNING id");
	stmt_pool.insert({	stmt::add_chunk,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::chunk, {"id"})}});

	// add instance.
	// instance fields are dynamic, unlike others.
	// fill text excluding `id` field name.
	auto& _instance_table  = schema.get_table_info(type::table::instance);
	auto  _names  = common::get_names(_instance_table.fields, {"id"}, false, "", false, "");	// `"f1","f2","f3"`
	auto  _values = common::get_names(_instance_table.fields, {"id"}, true,  "", false, "");	// `'?','?','?'`
	sql = schema.get_sql("INSERT INTO ${instance} (" + _names + ") VALUES (" + _values + ") RETURNING id");
	stmt_pool.insert({	stmt::add_instance,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::instance, {"id"})}});

	// add_instance_info.
	sql = schema.get_sql("INSERT INTO ${instance_info} (ref_instance_id, ref_chunk_id, weak_count) VALUES (?, ?, 0)");
	stmt_pool.insert({	stmt::add_instance_info,
						stmt_info{get_stmt(sql),
						{}}});

	// get_not_updated_instance.
	// instance fields are dynamic, unlike others.
	// fill text excluding `id` field name.
	sql = schema.get_sql(
		"SELECT ${instance}.* FROM ${instance} "
			"INNER JOIN " "${instance_info} " "on ${instance}.id " "= ${instance_info}.ref_instance_id "
			"INNER JOIN " "${chunk} "         "on ${chunk}.id "    "= ${instance_info}.ref_chunk_id " 
		"WHERE ${chunk}.updated=0"
	);
	stmt_pool.insert({	stmt::get_not_updated_instance,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::instance, {})}});

	// get chunk.
	sql = schema.get_sql("SELECT id FROM ${chunk} WHERE updated=?");
	stmt_pool.insert({	stmt::get_chunk,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::chunk, {"id"})}});

	// get_is_treenode_empty.
	sql = schema.get_sql("SELECT CASE WHEN EXISTS(SELECT 1 FROM ${generation}) THEN 0 ELSE 1 END AS emptiness");
	stmt_pool.insert({	stmt::get_is_generation_empty,
						stmt_info{get_stmt(sql),
						type::fields{{"emptiness", type::field_type::BIGINT}}}});

	// add generation.
	sql = schema.get_sql("INSERT INTO ${generation} (datetime, root_ref_treenode_id) VALUES (?,-1) RETURNING id");
	stmt_pool.insert({	stmt::add_generation,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::generation, {"id"})}});

	// update generation.
	sql = schema.get_sql("UPDATE ${generation} SET root_ref_treenode_id=? WHERE id=?");
	stmt_pool.insert({	stmt::update_generation,
						stmt_info{get_stmt(sql),
						{}}});

	// update generation_etc.
	sql = schema.get_sql("UPDATE ${generation} SET instance_count=?, weak_instance_count=?, weak_instance_ratio=?, before_weak_instance_accuracy=?,"
						 "after_weak_instance_accuracy=?, before_instance_accuracy=?, after_instance_accuracy=? WHERE id=?");
	stmt_pool.insert({	stmt::update_generation_etc,
						stmt_info{get_stmt(sql),
						{}}});

	// get_treenode_last_id.
	sql = schema.get_sql("SELECT id FROM ${treenode} WHERE id=(SELECT MAX(id) FROM ${treenode})");
	stmt_pool.insert({	stmt::get_treenode_last_id,
						stmt_info{get_stmt(sql), 
						schema.fields_include(type::table::treenode, {"id"})}});

	// add_rule.
	sql = schema.get_sql("INSERT INTO ${rule} (feature_index, rule_type, value_type, value_integer, value_real) VALUES (?,?,?,?,?) RETURNING id");
	stmt_pool.insert({	stmt::add_rule,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::rule, {"id"})}});

	// add leaf_info.
	sql = schema.get_sql("INSERT INTO ${leaf_info} (label_index, type, go_to_ref_generation_id, correct_count, total_count, accuracy) VALUES (?,?,?,?,?,?) RETURNING id");
	stmt_pool.insert({	stmt::add_leaf_node,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::leaf_info, {"id"})}});

	// add treenode.
	sql = schema.get_sql("INSERT INTO ${treenode} (ref_generation_id, ref_parent_treenode_id, ref_rule_id, ref_leaf_info_id) VALUES (?,?,?,?) RETURNING id");
	stmt_pool.insert({	stmt::add_treenode,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::treenode, {"id"})}});

	// get instances by chunk id.
	_names = common::get_names(_instance_table.fields, {}, false, "${instance}", true, "");	// `${instance}."f1" as "f1", ...`
	sql = schema.get_sql(
			"SELECT " + _names + " FROM ${instance} " +
				"INNER JOIN ${instance_info} " "ON ${instance}.id " "= ${instance_info}.ref_instance_id "
				"INNER JOIN ${chunk} "         "ON ${chunk}.id "    "= ${instance_info}.ref_chunk_id " +
			"WHERE ${chunk}.id=?"
	);
	stmt_pool.insert({	stmt::get_instance_by_chunk_id,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::instance, {})}});

	// get_first_root_ref_treenode_id.
	sql = schema.get_sql("SELECT root_ref_treenode_id FROM ${generation} ORDER BY id ASC LIMIT 1");
	stmt_pool.insert({	stmt::get_first_root_ref_treenode_id,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::generation, {"root_ref_treenode_id"})}});

	// get treenode.
	_names.clear();
	common::get_names_append(_names, schema.get_table_info(type::table::treenode).fields,	{},	false, "${treenode}",	true, "treenode");
	common::get_names_append(_names, schema.get_table_info(type::table::rule).fields,		{},	false, "${rule}",		true, "rule");
	common::get_names_append(_names, schema.get_table_info(type::table::leaf_info).fields,	{},	false, "${leaf_info}",	true, "leaf_info");
	sql = schema.get_sql("SELECT " + _names + " FROM ${treenode} "
							"LEFT JOIN ${rule} "	  "ON ${treenode}.ref_rule_id "      "= ${rule}.id "
							"LEFT JOIN ${leaf_info} " "ON ${treenode}.ref_leaf_info_id " "= ${leaf_info}.id "
							"WHERE ${treenode}.ref_parent_treenode_id = ?");
	stmt_pool.insert({	stmt::get_treenode,
						stmt_info{get_stmt(sql),
						schema.fields_join({
							{type::table::treenode,  {}},
							{type::table::rule,		 {}},
							{type::table::leaf_info, {}}}, true)}});

	// update instance_info.
	sql = schema.get_sql("UPDATE ${instance_info} SET ref_leaf_treenode_id=?, correct=? WHERE ref_instance_id=?");
	stmt_pool.insert({	stmt::update_instance_info,
						stmt_info{get_stmt(sql),
						{}}});

	// update chunk.
	sql = schema.get_sql("UPDATE ${chunk} SET updated=?, initial_correct_count=?, total_count=?, initial_accuracy=? WHERE id=?");
	stmt_pool.insert({	stmt::update_chunk,
						stmt_info{get_stmt(sql),
						{}}});

	// update chunk_total_count.
	sql = schema.get_sql("UPDATE ${chunk} SET total_count=? WHERE id=?");
	stmt_pool.insert({	stmt::update_chunk_total_count,
						stmt_info{get_stmt(sql),
						{}}});

	// get chunk_list.
	sql = schema.get_sql("SELECT * FROM ${chunk} ORDER BY datetime ASC");
	stmt_pool.insert({	stmt::get_chunk_list,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::chunk, {})}});

	// get chunk_updated.
	sql = schema.get_sql("SELECT updated FROM ${chunk} WHERE id=?");
	stmt_pool.insert({	stmt::get_chunk_updated,
						stmt_info{get_stmt(sql),
						type::fields{{"updated", type::field_type::TINYINT}}}});

	// update leaf_info.
	sql = schema.get_sql("UPDATE ${leaf_info} "
						 "SET correct_count=correct_count+?, "
							 "total_count=total_count+?, "
							 "accuracy=(CASE WHEN total_count+?=0 THEN 0.0 ELSE (CAST(correct_count AS REAL)+?)/(total_count+?) END) "
						 "WHERE id=?");
	stmt_pool.insert({	stmt::update_leaf_info,
						stmt_info{get_stmt(sql),
						{}}});

	// find weak treenode.
	// remark)
	// - leaf_info.type = 1(=leaf_info_type::leaf)
	// - leaf_info.type is not included in the index,
	//   but seach combined with AND is optimized.
	//   (https://www.sqlite.org/queryplanner.html)
	sql = schema.get_sql("SELECT ${treenode}.id AS \"treenode.id\" FROM ${leaf_info} "
							"INNER JOIN ${treenode} ON ${treenode}.ref_leaf_info_id = ${leaf_info}.id "
						 "WHERE ${leaf_info}.accuracy <= ? AND ${leaf_info}.total_count >= ? AND ${leaf_info}.type = 1");
	stmt_pool.insert({	stmt::get_weak_treenode,
						stmt_info{get_stmt(sql),
						schema.fields_join({
							{type::table::treenode, {"id"}},
						}, false)}});

	// update_leaf_info_by_go_to_generation_id.
	sql = schema.get_sql("UPDATE ${leaf_info} SET type=2, go_to_ref_generation_id=? "
						 "WHERE ${leaf_info}.accuracy <= ? AND ${leaf_info}.total_count >= ? AND ${leaf_info}.type = 1");
	stmt_pool.insert({	stmt::update_leaf_info_by_go_to_generation_id,
						stmt_info{get_stmt(sql),
						{}}});

	// get instance_by_go_to_generation_id.
	sql = schema.get_sql("SELECT ${instance}.*, ${leaf_info}.label_index as \"leaf_info.label_index\" FROM ${instance} "
							"INNER JOIN ${instance_info} ON ${instance_info}.ref_instance_id = ${instance}.id "
							"INNER JOIN ${treenode} "   "ON ${treenode}.id "				"= ${instance_info}.ref_leaf_treenode_id "
							"INNER JOIN ${leaf_info} "  "ON ${leaf_info}.id "				"= ${treenode}.ref_leaf_info_id "
						 "WHERE ${leaf_info}.go_to_ref_generation_id=?");
	stmt_pool.insert({	stmt::get_instance_by_go_to_generation_id,
						stmt_info{get_stmt(sql),
						schema.fields_merge({
							schema.fields_include(type::table::instance, {}),
							type::fields{{"leaf_info.label_index", schema.field_type(type::table::leaf_info, "label_index")}}})
						}});

	// get correct_instance_count_by_go_to_generation_id.
	sql = schema.get_sql("SELECT COUNT(*) FROM ${instance} "
							"INNER JOIN ${instance_info} ON ${instance_info}.ref_instance_id = ${instance}.id "
							"INNER JOIN ${treenode} "   "ON ${treenode}.id "				"= ${instance_info}.ref_leaf_treenode_id "
							"INNER JOIN ${leaf_info} "  "ON ${leaf_info}.id "				"= ${treenode}.ref_leaf_info_id "
						 "WHERE ${leaf_info}.go_to_ref_generation_id=? AND ${instance_info}.correct=1");
	stmt_pool.insert({	stmt::get_correct_instance_count_by_go_to_generation_id,
						stmt_info{get_stmt(sql),
						type::fields{{"COUNT(*)", type::field_type::BIGINT}}}});

	// update increment weak_count.
	sql = schema.get_sql("UPDATE ${instance_info} SET ref_leaf_treenode_id=?, correct=?, weak_count = weak_count + 1 WHERE ref_instance_id=?");
	stmt_pool.insert({	stmt::update_instance_info_with_weak_count_increment,
						stmt_info{get_stmt(sql),
						{}}});

	// get root_ref_treenode_id.
	sql = schema.get_sql("SELECT root_ref_treenode_id FROM ${generation} WHERE id=?");
	stmt_pool.insert({	stmt::get_root_ref_treenode_id,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::generation, {"root_ref_treenode_id"})}});

	// copy rule.
	auto& _rule_table = schema.get_table_info(type::table::rule);
	_names  = common::get_names(_rule_table.fields, {"id"}, false, "", false, "");	// "feature_index","rule_type",...
	sql = schema.get_sql("INSERT INTO ${rule} (" + _names + ") "
						 "SELECT " + _names + " "
						 "FROM ${rule} "
						 "WHERE id=? "
						 "RETURNING id");
	stmt_pool.insert({	stmt::copy_rule,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::rule, {"id"})}});

	// update rule (value_integer).
	sql = schema.get_sql("UPDATE ${rule} SET value_integer=? WHERE id=?");
	stmt_pool.insert({	stmt::update_rule_value_integer,
						stmt_info{get_stmt(sql),
						{}}});

	// get generation_id_by_treenode_id.
	sql = schema.get_sql("SELECT ref_generation_id FROM ${treenode} WHERE id=?");
	stmt_pool.insert({	stmt::get_generation_id_by_treenode_id,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::treenode, {"ref_generation_id"})}});

	// get leaf_info_by_chunk_id.
	auto& _leaf_info_table = schema.get_table_info(type::table::leaf_info);
	_names = common::get_names(_leaf_info_table.fields, {}, false, "${leaf_info}", true, "");	// ${leaf_info}."f1" as "f1", ...
	sql = schema.get_sql("SELECT "
							"${instance}.\"%y%\" AS \"instance.actual\", " 
							"${instance_info}.weak_count AS \"instance_info.weak_count\", "
							+ _names + " "
						 "FROM ${instance_info} "
							"INNER JOIN ${instance} ON ${instance_info}.id = ${instance}.id "
							"INNER JOIN ${treenode} ON ${instance_info}.ref_leaf_treenode_id = ${treenode}.id "
							"INNER JOIN ${leaf_info} ON ${treenode}.ref_leaf_info_id = ${leaf_info}.id "
						 "WHERE ${instance_info}.ref_chunk_id=?");
	stmt_pool.insert({	stmt::get_leaf_info_by_chunk_id,
						stmt_info{get_stmt(sql),
						schema.fields_merge({
						type::fields{
							{"instance.actual", type::field_type::INTEGER},
							{"instance_info.weak_count", schema.field_type(type::table::instance_info, "weak_count")}
						},
						schema.fields_include(type::table::leaf_info, {})})}});

	// get total_count_by_chunk_id.
	sql = schema.get_sql("SELECT total_count FROM ${chunk} WHERE id=?");
	stmt_pool.insert({	stmt::get_total_count_by_chunk_id,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::leaf_info, {"total_count"})}});

	// delete instance_by_chunk_id.
	sql = schema.get_sql("DELETE FROM ${instance} "
						 "WHERE id IN("
							"SELECT ${instance_info}.id "
							"FROM ${instance_info} "
							"INNER JOIN ${chunk} ON ${instance_info}.ref_chunk_id = ${chunk}.id "
							"WHERE ${chunk}.id=?"
						 ")");
	stmt_pool.insert({	stmt::delete_instance_by_chunk_id,
						stmt_info{get_stmt(sql),
						{}}});

	// delete instance_info_by_chunk_id.
	sql = schema.get_sql("DELETE FROM ${instance_info} "
						 "WHERE id IN("
							"SELECT ${instance_info}.id "
							"FROM ${instance_info} "
							"INNER JOIN ${chunk} ON ${instance_info}.ref_chunk_id = ${chunk}.id "
							"WHERE ${chunk}.id=?"
						 ")");
	stmt_pool.insert({	stmt::delete_instance_info_by_chunk_id,
						stmt_info{get_stmt(sql),
						{}}});

	// delete chunk_by_id.
	sql = schema.get_sql("DELETE FROM ${chunk} WHERE id=?");
	stmt_pool.insert({	stmt::delete_chunk_by_id,
						stmt_info{get_stmt(sql),
						{}}});

	// get global_row_count.
	sql = schema.get_sql("SELECT COUNT(*) FROM ${global}");
	stmt_pool.insert({	stmt::get_global_row_count,
						stmt_info{get_stmt(sql),
						type::fields{{"COUNT(*)", type::field_type::BIGINT}}}});

	// add global_one_row.
	// only one row as id = 1.
	sql = schema.get_sql("INSERT INTO ${global} (id) VALUES (1)");
	stmt_pool.insert({	stmt::add_global_one_row,
						stmt_info{get_stmt(sql),
						{}}});

	// get global.
	sql = schema.get_sql("SELECT * FROM ${global} WHERE id=1");
	stmt_pool.insert({	stmt::get_global,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::global, {})}});

	// get instance_count.
	sql = schema.get_sql("SELECT COUNT(*) FROM ${instance_info}");
	stmt_pool.insert({	stmt::get_instance_count,
						stmt_info{get_stmt(sql),
						type::fields{{"COUNT(*)", type::field_type::BIGINT}}}});

	// get instance_correct_count.
	sql = schema.get_sql("SELECT COUNT(*) FROM ${instance_info} WHERE correct=1");
	stmt_pool.insert({	stmt::get_instance_correct_count,
						stmt_info{get_stmt(sql),
						type::fields{{"COUNT(*)", type::field_type::BIGINT}}}});

	// get updated_instance_count.
	sql = schema.get_sql("SELECT COUNT(*) FROM ${instance} "
						 "INNER JOIN ${instance_info} ON ${instance}.id = ${instance_info}.ref_instance_id "
						 "INNER JOIN ${chunk} ON ${chunk}.id = ${instance_info}.ref_chunk_id "
						 "WHERE ${chunk}.updated=1");
	stmt_pool.insert({	stmt::get_updated_instance_count,
						stmt_info{get_stmt(sql),
						type::fields{{"COUNT(*)", type::field_type::BIGINT}}}});

	// get sum_leaf_info_total_count.
	sql = schema.get_sql("SELECT SUM(${leaf_info}.total_count) AS result FROM ${treenode} "
						 "INNER JOIN ${leaf_info} ON ${leaf_info}.id = ${treenode}.ref_leaf_info_id "
						 "WHERE ${leaf_info}.type = 1");
	stmt_pool.insert({	stmt::get_sum_leaf_info_total_count,
						stmt_info{get_stmt(sql),
						type::fields{{"result", type::field_type::BIGINT}}}});

	// get sum_weak_count.
	sql = schema.get_sql("SELECT SUM(weak_count) FROM ${instance_info}");
	stmt_pool.insert({	stmt::get_sum_weak_count,
						stmt_info{get_stmt(sql),
						type::fields{{"SUM(weak_count)", type::field_type::BIGINT}}}});

	// get instance_actual_predicted.
	sql = schema.get_sql("SELECT ${instance}.\"%y%\" AS actual, ${leaf_info}.label_index AS predicted FROM ${instance} "
						 "INNER JOIN ${instance_info} ON ${instance_info}.ref_instance_id = ${instance}.id "
						 "INNER JOIN ${treenode} ON ${treenode}.id = ${instance_info}.ref_leaf_treenode_id "
						 "INNER JOIN ${leaf_info} ON ${leaf_info}.id = ${treenode}.ref_leaf_info_id");
	stmt_pool.insert({	stmt::get_instance_actual_predicted,
						stmt_info{get_stmt(sql),
						type::fields{{"actual",		type::field_type::INTEGER},
									 {"predicted",	type::field_type::INTEGER}}}});

	// get global_confusion_matrix.
	sql = schema.get_sql("SELECT * FROM ${global_confusion_matrix}");
	stmt_pool.insert({	stmt::get_global_confusion_matrix,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::global_confusion_matrix, {})}});

	// get global_confusion_matrix_item_count.
	sql = schema.get_sql("SELECT COUNT(id) FROM ${global_confusion_matrix} WHERE actual=? AND predicted=?");
	stmt_pool.insert({	stmt::get_global_confusion_matrix_item_count,
						stmt_info{get_stmt(sql),
						type::fields{{"SUM(weak_count)", type::field_type::BIGINT}}}});

	// add global_confusion_matrix_item.
	sql = schema.get_sql("INSERT INTO ${global_confusion_matrix} (actual, predicted) VALUES (?, ?)");
	stmt_pool.insert({	stmt::add_global_confusion_matrix_item,
						stmt_info{get_stmt(sql),
						{}}});

	// update global_confusion_matrix_item_increment.
	sql = schema.get_sql("UPDATE ${global_confusion_matrix} SET count = coalesce(count, 0) + ? WHERE actual = ? AND predicted = ? RETURNING id");
	stmt_pool.insert({	stmt::update_global_confusion_matrix_item_increment,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::global_confusion_matrix, {"id"})}});

	// get chunk_initial_accuracy.
	sql = schema.get_sql("SELECT initial_accuracy FROM ${chunk}");
	stmt_pool.insert({	stmt::get_chunk_initial_accuracy,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::chunk, {"initial_accuracy"})}});

	// get chunk_last_id.
	sql = schema.get_sql("SELECT id FROM ${chunk} ORDER BY id DESC LIMIT 1");
	stmt_pool.insert({	stmt::get_chunk_last_id,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::chunk, {"id"})}});

	// get chunk_by_id.
	sql = schema.get_sql("SELECT * FROM ${chunk} WHERE id = ?");
	stmt_pool.insert({	stmt::get_chunk_by_id,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::chunk, {})}});

	// get chunk_for_report.
	sql = schema.get_sql("SELECT * FROM ${chunk} WHERE updated = 1");
	stmt_pool.insert({	stmt::get_chunk_for_report,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::chunk, {})}});

	// get generation_for_report.
	sql = schema.get_sql("SELECT * FROM ${generation}");
	stmt_pool.insert({	stmt::get_generation_for_report,
						stmt_info{get_stmt(sql),
						schema.fields_include(type::table::generation, {})}});

	// the result fields of the query are set manually.
	// verify with the count.
	// column names(sqlite3_column_name()) and types(sqlite3_column_decltype()) are excluded from verification
	// because they can return NULL sometimes.
	for (auto& pool: stmt_pool) {
		auto& stmt_type = pool.first;
		auto& stmt_info = pool.second;
		if (not stmt_info.stmt) THROW_SUPUL_INTERNAL_ERROR0;
		int count = sqlite3_column_count(stmt_info.stmt);
		if (static_cast<int>(stmt_info.fields.size()) != count) THROW_SUPUL_INTERNAL_ERROR0;
	}
}

inline int sqlite_t::count_string_table(void) {
	auto result = execute(stmt::count_string_table, {}, true);
	if (result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
	return common::get_variant_int(result, "COUNT(*)");
}

inline int sqlite_t::get_string_table_last_id(void) {
	auto result = execute(stmt::get_string_table_last_id, {}, true);
	if (result.empty()) return 0;
	return common::get_variant_int(result, "id");
}

inline void sqlite_t::get_string_table(_in callback_query cb) {
	execute(stmt::get_string_table, {}, cb);
}

inline void sqlite_t::add_string_table(_in int id, _in const std::string& text) {
	execute(stmt::add_string_table, {static_cast<int64_t>(id), text}, true);
}

inline int64_t sqlite_t::add_chunk(_in int64_t datetime) {
	auto result = execute(stmt::add_chunk, {datetime}, true);
	return common::get_variant_int64(result, "id");
}

inline int64_t sqlite_t::add_instance(_in const type::vector_variant& params) {
	auto result = execute(stmt::add_instance, params, true);
	return common::get_variant_int64(result, "id");
}

inline void sqlite_t::add_instance_info(_in int64_t instance_id, _in int64_t chunk_id) {
	execute(stmt::add_instance_info, {instance_id, chunk_id}, true);
}

inline auto sqlite_t::get_not_updated_instance(void) -> gaenari::dataset::dataframe {
	gaenari::dataset::dataframe df;
	df = execute(stmt::get_not_updated_instance, {});
	return df;
}

inline void sqlite_t::get_instance_by_chunk_id(_in int64_t chunk_id, _in callback_query cb) {
	execute(stmt::get_instance_by_chunk_id, {chunk_id}, cb);
}

inline auto sqlite_t::get_chunk(_in bool updated) -> std::vector<int64_t> {
	int64_t _updated = updated ? 1 : 0;
	return execute<int64_t>(stmt::get_chunk, {_updated}, "id");
}

inline bool sqlite_t::get_is_generation_empty(void) {
	auto result = execute(stmt::get_is_generation_empty, {}, true);
	if (result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
	if (1 == common::get_variant_int(result, "emptiness")) return true;
	return false;
}

inline int64_t sqlite_t::add_generation(_in int64_t datetime) {
	auto result = execute(stmt::add_generation, {datetime}, true);
	return common::get_variant_int64(result, "id");
}

inline void sqlite_t::update_generation(_in int64_t generation_id, _in int64_t root_ref_treenode_id) {
	auto result = execute(stmt::update_generation, {root_ref_treenode_id, generation_id}, true);	// order changed!
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline void sqlite_t::update_generation_etc(_in int64_t generation_id, _in int64_t instance_count, _in int64_t weak_instance_count,
	_in double weak_instance_ratio, _in double before_weak_instance_accuracy, _in double after_weak_instance_accuracy,
	_in double before_instance_accuracy, _in double after_instance_accuracy) {
	auto result = execute(stmt::update_generation_etc, {instance_count, weak_instance_count,
					weak_instance_ratio, before_weak_instance_accuracy, after_weak_instance_accuracy, 
					before_instance_accuracy, after_instance_accuracy, generation_id}, true);	// order changed!
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline int64_t sqlite_t::get_treenode_last_id(void) {
	auto result = execute(stmt::get_treenode_last_id, {}, true);
	if (result.empty()) return 0;
	return common::get_variant_int(result, "id");
}

inline int64_t sqlite_t::add_rule(_in int feature_index, _in int rule_type, _in int value_type, _in int64_t value_int, _in double value_double) {
	auto result = execute(stmt::add_rule, {
							static_cast<int64_t>(feature_index),
							static_cast<int64_t>(rule_type),
							static_cast<int64_t>(value_type),
							static_cast<int64_t>(value_int),
							value_double}, 
						  true);
	return common::get_variant_int64(result, "id");
}

inline int64_t sqlite_t::add_leaf_info(_in int label_index, _in type::leaf_info_type type, _in int64_t go_to_ref_generation_id, _in int64_t correct_count, _in int64_t total_count, _in double accuracy) {
	auto result = execute(stmt::add_leaf_node, {
							static_cast<int64_t>(label_index),
							static_cast<int64_t>(type),
							go_to_ref_generation_id,
							correct_count,
							total_count,
							accuracy},
						  true);
	return common::get_variant_int64(result, "id");
}

inline int64_t sqlite_t::add_treenode(_in int64_t ref_generation_id, _in int64_t ref_parent_treenode_id, _in int64_t ref_rule_id, _in int64_t ref_leaf_info_id) {
	auto result = execute(stmt::add_treenode, {ref_generation_id, ref_parent_treenode_id, ref_rule_id, ref_leaf_info_id}, true);
	return common::get_variant_int64(result, "id");
}

inline int64_t sqlite_t::get_first_root_ref_treenode_id(void) {
	auto result = execute(stmt::get_first_root_ref_treenode_id, {}, true);
	return common::get_variant_int64(result, "root_ref_treenode_id");
}

inline auto sqlite_t::get_treenode(_in int64_t parent_treenode_id) -> std::vector<type::treenode_db> {
	std::vector<type::treenode_db> ret;
	execute(stmt::get_treenode, {parent_treenode_id}, [&ret](const auto& row) -> bool {
		type::treenode_db treenode;
		int64_t ref_leaf_info_id = 0;

		// row record -> treenode_db.
		common::get_variant(row, "treenode.id",					treenode.id);
		common::get_variant(row, "rule.id",						treenode.rule.id);
		common::get_variant(row, "rule.feature_index",			treenode.rule.feature_index);
		common::get_variant(row, "rule.rule_type",				treenode.rule.rule_type);
		common::get_variant(row, "rule.value_type",				treenode.rule.value_type);
		common::get_variant(row, "rule.value_integer",			treenode.rule.value_integer);
		common::get_variant(row, "rule.value_real",				treenode.rule.value_real);

		// leaf info.
		common::get_variant(row, "treenode.ref_leaf_info_id",	ref_leaf_info_id, true);
		if (ref_leaf_info_id >= 0) {
			// it is a leaf node.
			int64_t type = 0;
			treenode.is_leaf_node = true;
			common::get_variant(row, "leaf_info.id",						treenode.leaf_info.id);
			common::get_variant(row, "leaf_info.label_index",				treenode.leaf_info.label_index);
			common::get_variant(row, "leaf_info.type",						type);
			common::get_variant(row, "leaf_info.go_to_ref_generation_id",	treenode.leaf_info.go_to_ref_generation_id);
			common::get_variant(row, "leaf_info.correct_count",				treenode.leaf_info.correct_count);
			common::get_variant(row, "leaf_info.total_count",				treenode.leaf_info.total_count);
			common::get_variant(row, "leaf_info.accuracy",					treenode.leaf_info.accuracy);
			treenode.leaf_info.type = static_cast<type::leaf_info_type>(type);
		} else if (ref_leaf_info_id == -1) {
			// it is not a leaf node.
			// do nothing.
		} else {
			THROW_SUPUL_INTERNAL_ERROR0;
		}

		ret.emplace_back(treenode);
		return true;
	});

	return ret;
}

inline void sqlite_t::update_instance_info(_in int64_t instance_id, _in int64_t ref_leaf_treenode_id, _in bool correct) {
	auto result = execute(stmt::update_instance_info, {ref_leaf_treenode_id, static_cast<int64_t>(correct), instance_id}, true);	// order changed!
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline void sqlite_t::update_chunk(_in int64_t chunk_id, _in bool updated, _in int64_t initial_correct_count, _in int64_t total_count, _in double initial_accuracy) {
	auto result = execute(stmt::update_chunk, {static_cast<int64_t>(updated), initial_correct_count, total_count, initial_accuracy, chunk_id}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline void sqlite_t::update_chunk_total_count(_in int64_t chunk_id, _in int64_t total_count) {
	auto result = execute(stmt::update_chunk_total_count, {total_count, chunk_id}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline void	sqlite_t::get_chunk_list(_in callback_query cb) {
	execute(stmt::get_chunk_list, {}, cb);
}

inline bool	sqlite_t::get_chunk_updated(_in int64_t chunk_id) {
	auto result = execute(stmt::get_chunk_updated, {chunk_id}, true);
	if (common::get_variant_int64(result, "updated") == 1) return true;
	return false;
}

inline void sqlite_t::update_leaf_info(_in int64_t leaf_info_id, _in int64_t increment_correct_count, _in int64_t increment_total_count) {
	auto result = execute(stmt::update_leaf_info, {increment_correct_count, increment_total_count, increment_total_count, increment_correct_count, increment_total_count, leaf_info_id}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline auto sqlite_t::get_weak_treenode(_in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound) -> std::vector<int64_t> {
	gaenari::common::elapsed_time_logger t("sqlite_t::get_weak_treenode()");
	return execute<int64_t>(stmt::get_weak_treenode, {leaf_node_accuracy_upperbound, leaf_node_total_count_lowerbound}, "treenode.id");
}

inline void sqlite_t::update_leaf_info_by_go_to_generation_id(_in int64_t generation_id, _in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound) {
	gaenari::common::elapsed_time_logger t("sqlite_t::update_leaf_info_by_go_to_generation_id()");
	auto result = execute(stmt::update_leaf_info_by_go_to_generation_id, {generation_id, leaf_node_accuracy_upperbound, leaf_node_total_count_lowerbound}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline auto sqlite_t::get_instance_by_go_to_generation_id(_in int64_t go_to_ref_generation_id) -> gaenari::dataset::dataframe {
	// TO_DO:
	// : if we are unlucky, it may be allocated very large dataframe.
	//   a capacity limit is required.
	gaenari::dataset::dataframe df;
	df = execute(stmt::get_instance_by_go_to_generation_id, {go_to_ref_generation_id});
	return df;
}

inline auto sqlite_t::get_correct_instance_count_by_go_to_generation_id(_in int64_t go_to_ref_generation_id) -> int64_t {
	gaenari::common::elapsed_time_logger t("sqlite_t::get_correct_instance_count_by_go_to_generation_id()");
	auto result = execute(stmt::get_correct_instance_count_by_go_to_generation_id, {go_to_ref_generation_id}, true);
	return common::get_variant_int64(result, "COUNT(*)");
}

inline void sqlite_t::update_instance_info_with_weak_count_increment(_in int64_t instance_id, _in int64_t ref_leaf_treenode_id, _in bool correct) {
	auto result = execute(stmt::update_instance_info_with_weak_count_increment, {ref_leaf_treenode_id, static_cast<int64_t>(correct), instance_id}, true);	// order changed!
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline int64_t sqlite_t::get_root_ref_treenode_id(_in int64_t generation_id) {
	auto result = execute(stmt::get_root_ref_treenode_id, {generation_id}, true);
	return common::get_variant_int64(result, "root_ref_treenode_id");
}

inline int64_t sqlite_t::copy_rule(_in int64_t src_rule_id) {
	auto result = execute(stmt::copy_rule, {src_rule_id}, true);
	return common::get_variant_int64(result, "id");
}

inline void sqlite_t::update_rule_value_integer(_in int64_t rule_id, _in int64_t value_integer) {
	auto result = execute(stmt::update_rule_value_integer, {value_integer, rule_id}, true);	// order chagned!
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline int64_t sqlite_t::get_generation_id_by_treenode_id(_in int64_t treenode_id) {
	auto result = execute(stmt::get_generation_id_by_treenode_id, {treenode_id}, true);
	return common::get_variant_int64(result, "ref_generation_id");
}

inline void sqlite_t::get_leaf_info_by_chunk_id(_in int64_t chunk_id, _in callback_query cb) {
	execute(stmt::get_leaf_info_by_chunk_id, {chunk_id}, cb);
}

inline int64_t sqlite_t::get_total_count_by_chunk_id(_in int64_t chunk_id) {
	auto result = execute(stmt::get_total_count_by_chunk_id, {chunk_id}, true);
	return common::get_variant_int64(result, "total_count");
}

inline void	sqlite_t::delete_instance_by_chunk_id(_in int64_t chunk_id) {
	auto result = execute(stmt::delete_instance_by_chunk_id, {chunk_id}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline void	sqlite_t::delete_instance_info_by_chunk_id(_in int64_t chunk_id) {
	auto result = execute(stmt::delete_instance_info_by_chunk_id, {chunk_id}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline void	sqlite_t::delete_chunk_by_id(_in int64_t chunk_id) {
	auto result = execute(stmt::delete_chunk_by_id, {chunk_id}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline int64_t sqlite_t::get_global_row_count(void) {
	auto result = execute(stmt::get_global_row_count, {}, true);
	return common::get_variant_int64(result, "COUNT(*)");
}

inline void sqlite_t::add_global_one_row(void) {
	auto result = execute(stmt::add_global_one_row, {}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

inline auto sqlite_t::get_global(void) -> type::map_variant {
	auto result = execute(stmt::get_global, {}, true);
	if (result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
	return result;
}

inline void sqlite_t::set_global(_in const type::map_variant& g, _option_in bool increment) {
	// get fields of global.
	auto& table_info = schema.get_table_info(type::table::global);
	auto& fields = table_info.fields;

	// check error.
	if (g.empty()) THROW_SUPUL_INTERNAL_ERROR0;

	// UPDATE global SET key=value, [...] WHERE id = 1
	std::string sql = "UPDATE ${global} SET ";
	for (const auto& it: g) {
		const auto& name = it.first;
		const auto index = it.second.index();
		const auto field_find = fields.find(name);
		if (field_find == fields.end()) THROW_SUPUL_ERROR("invalid parameter: " + name);

		// get field type.
		const auto& field_type = field_find->second;

		// check for errors in advance.
		switch (field_type) {
		case type::field_type::INTEGER: case type::field_type::BIGINT: case type::field_type::SMALLINT:
		case type::field_type::TINYINT: case type::field_type::TEXT_ID:
			if (not ((index == 1) or (index == 2))) THROW_SUPUL_INTERNAL_ERROR0;
			break;
		case type::field_type::REAL:
			if (not ((index == 1) or (index == 2))) THROW_SUPUL_INTERNAL_ERROR0;
			break;
		case type::field_type::TEXT:
			if (not (index == 3)) THROW_SUPUL_INTERNAL_ERROR0;
			break;
		default:
			THROW_SUPUL_INTERNAL_ERROR0;
		}

		if (increment)	sql += name + '=' + name + "+?,";
		else			sql += name + "=?,";
	}
	sql.pop_back();
	sql += " WHERE id=1";

	// get sql.
	sql = schema.get_sql(sql);
	
	// prepare statement.
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare(this->db, sql.c_str(), -1, &stmt, nullptr);
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to sqlite3_prepare.", error_desc(rc));

	// bind.
	int pos = 1;
	for (const auto& it: g) {
		const auto& name  = it.first;
		const auto& value = it.second;
		const auto index  = value.index();
		const auto field_find = fields.find(name);
		if (field_find == fields.end()) {
			sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			THROW_SUPUL_ERROR("invalid parameter: " + name);
		}

		// get field type.
		const auto& field_type = field_find->second;

		rc = SQLITE_ERROR;
		switch (field_type) {
		case type::field_type::INTEGER:
		case type::field_type::BIGINT:
		case type::field_type::SMALLINT:
		case type::field_type::TINYINT:
		case type::field_type::TEXT_ID:
			if (index == 1)	{			// int64_t.
				rc = sqlite3_bind_int64(stmt, pos, std::get<1>(value));
			} else if (index == 2) {	// double.
				rc = sqlite3_bind_int64(stmt, pos, static_cast<int64_t>(std::get<2>(value)));
			}
			break;
		case type::field_type::REAL:
			if (index == 1) {			// int64_t.
				rc = sqlite3_bind_double(stmt, pos, static_cast<double>(std::get<1>(value)));
			} else if (index == 2) {	// double.
				rc = sqlite3_bind_double(stmt, pos, std::get<2>(value));
			}
			break;
		case type::field_type::TEXT:
			// std::string.
			rc = sqlite3_bind_text(stmt, pos, std::get<3>(value).c_str(), -1, SQLITE_STATIC);
			break;
		default:
			sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			THROW_SUPUL_INTERNAL_ERROR0;
		}

		if (rc != SQLITE_OK) {
			auto desc = error_desc(rc);
			sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			THROW_SUPUL_DB_ERROR("fail to bind.", desc);
		}

		pos++;
	}

	// step.
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		auto desc = error_desc(rc);
		sqlite3_reset(stmt);
		sqlite3_finalize(stmt);
		THROW_SUPUL_DB_ERROR("fail to sqlite3_step.", desc);
	}

	// reset.
	rc = sqlite3_reset(stmt);
	if (rc != SQLITE_OK) {
		auto desc = error_desc(rc);
		sqlite3_finalize(stmt);
		THROW_SUPUL_DB_ERROR("fail to sqlite3_reset.", desc);
	}

	// finalize.
	rc = sqlite3_finalize(stmt);
	if (rc != SQLITE_OK) THROW_SUPUL_DB_ERROR("fail to sqlite3_finalize.", error_desc(rc));
}

inline int64_t sqlite_t::get_instance_count(void) {
	auto result = execute(stmt::get_instance_count, {}, true);
	return common::get_variant_int64(result, "COUNT(*)");
}

inline int64_t sqlite_t::get_instance_correct_count(void) {
	auto result = execute(stmt::get_instance_correct_count, {}, true);
	return common::get_variant_int64(result, "COUNT(*)");
}

inline int64_t sqlite_t::get_updated_instance_count(void) {
	auto result = execute(stmt::get_updated_instance_count, {}, true);
	return common::get_variant_int64(result, "COUNT(*)");
}

inline int64_t sqlite_t::get_sum_leaf_info_total_count(void) {
	auto result = execute(stmt::get_sum_leaf_info_total_count, {}, true);
	return common::get_variant_int64(result, "result", false);
}

inline int64_t sqlite_t::get_sum_weak_count(void) {
	auto result = execute(stmt::get_sum_weak_count, {}, true);
	return common::get_variant_int64(result, "SUM(weak_count)", false);
}

inline void sqlite_t::get_instance_actual_predicted(_in callback_query cb) {
	execute(stmt::get_instance_actual_predicted, {}, cb);
}

inline auto sqlite_t::get_global_confusion_matrix(void) -> std::vector<type::map_variant> {
	std::vector<type::map_variant> ret;
	execute(stmt::get_global_confusion_matrix, {}, ret);
	return ret;
}

inline int64_t sqlite_t::get_global_confusion_matrix_item_count(_in int64_t actual, _in int64_t predicted) {
	auto result = execute(stmt::get_global_confusion_matrix_item_count, {actual, predicted}, true);
	return common::get_variant_int64(result, "COUNT(id)");
}

inline void sqlite_t::add_global_confusion_matrix_item(_in int64_t actual, _in int64_t predicted) {
	auto result = execute(stmt::add_global_confusion_matrix_item, {actual, predicted}, true);
	if (not result.empty()) THROW_SUPUL_INTERNAL_ERROR0;
}

// especially returns bool.
inline bool sqlite_t::update_global_confusion_matrix_item_increment(_in int64_t actual, _in int64_t predicted, _in int64_t increment) {
	std::vector<type::map_variant> ret;
	execute(stmt::update_global_confusion_matrix_item_increment, {increment, actual, predicted}, ret);
	if (ret.empty()) return false;
	if (ret.size() == 1) return true;
	THROW_SUPUL_INTERNAL_ERROR0;
}

inline auto sqlite_t::get_chunk_initial_accuracy(void) -> std::vector<double> {
	return execute<double>(stmt::get_chunk_initial_accuracy, {}, "initial_accuracy");
}

inline int64_t sqlite_t::get_chunk_last_id(void) {
	auto result = execute(stmt::get_chunk_last_id, {}, true);
	return common::get_variant_int64(result, "id");
}

inline auto sqlite_t::get_chunk_by_id(_in int64_t id) -> type::map_variant {
	return execute(stmt::get_chunk_by_id, {id}, true);
}

inline auto sqlite_t::get_chunk_for_report(void) -> std::unordered_map<std::string, std::vector<type::value_variant>> {
	std::unordered_map<std::string, std::vector<type::value_variant>> ret;
	return execute_column_based(stmt::get_chunk_for_report, {});
}

inline auto sqlite_t::get_generation_for_report(void)->std::unordered_map<std::string, std::vector<type::value_variant>> {
	std::unordered_map<std::string, std::vector<type::value_variant>> ret;
	return execute_column_based(stmt::get_generation_for_report, {});
}

} // sqlite
} // db
} // supul

#endif // HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_IMPL_HPP
