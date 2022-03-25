#ifndef HEADER_GAENARI_SUPUL_DB_BASE_HPP
#define HEADER_GAENARI_SUPUL_DB_BASE_HPP

namespace supul {
namespace db {

// it is difficult to know how the new database driver will be implemented.
// therefore, the functions are implemented as a scenarios list rather than abstractly implemented as a sql query.
// in other words, it is not forced to implement as a database using sql.
// if you implement all the functions below, you can create a new driver.
class base {
public:
	base() = delete;
	base(const db::schema& schema): schema{schema} {}
	virtual ~base() = default;

public:
	// required to do some work like creating tables and indexes.
	// see sqlite's init() for a new driver implementation.
	virtual void init(_in const gaenari::common::prop& property, _in const type::paths& paths) = 0;
	virtual void deinit(void) = 0;

	// schema information from caller.
protected:
	// const db::schema& schema;
	const db::schema& schema;

	// callbacks.
	//  - pass args to caller.
	//  - return value from caller.

	// pass (fieldname, value) map to caller.
	using callback_query = std::function<bool(const type::map_variant& row)>;

	// database transaction api.
public:
	// if a write operation is scheduled,
	// set `exclusive` to fail in advance when another write transaction is already in progress.
	// (in practice, it does not fail immediately, but fails after waiting for lock_timeout().)
	// rollback() must be implemented because it can be called in error-free situations as well as internal errors.
	virtual void begin(_in bool exclusive) = 0;
	virtual void commit(void) = 0;
	virtual void rollback(void) = 0;
	virtual void lock_timeout(_in int msec) = 0;

	// supul database api.
	// for details, refer to the sqlite::* code.
	// here is an example of prepared statements in sqlite.
public:
	// returns the number of stored string table.
	// SELECT COUNT(*) FROM "string_table"
	virtual int	count_string_table(void) = 0;

	// returns the last id value of the string table.
	// SELECT id FROM "string_table" WHERE id=(SELECT MAX(id) FROM "string_table")
	virtual int  get_string_table_last_id(void) = 0;

	// query the string table and call each row as a callback.
	// SELECT * FROM "string_table"
	virtual void get_string_table(_in callback_query cb) = 0;

	// stores (id, text) values in string table.
	// INSERT INTO "string_table" (id, text) VALUES (?,?)
	virtual void add_string_table(_in int id, _in const std::string& text) = 0;

	// add chunk and return id.
	// INSERT INTO "chunk" (datetime, updated) VALUES (?, 0) RETURNING id
	virtual int64_t add_chunk(_in int64_t datetime) = 0;

	// add instance and return id.
	// fieldnames are defined in attributes.json.
	// INSERT INTO "instance" ("f1","f2",...) VALUES (?,?,...) RETURNING id
	virtual int64_t add_instance(_in const type::vector_variant& params) = 0;

	// add instance_info.
	// INSERT INTO "instance_info" (ref_instance_id, ref_chunk_id) VALUES (?, ?)
	virtual void add_instance_info(_in int64_t instance_id, _in int64_t chunk_id) = 0;

	// get not updated instances as dataframe.
	// you may need to call set_string_table_reference_from(...) to stringfy dataframe.
	// fieldnames are defined in attributes.json.
	// SELECT "instance".* FROM "instance"
	//     INNER JOIN "instance_info" ON "instance".id = "instance_info".ref_instance_id
	//     INNER JOIN "chunk"         ON "chunk".id    = "instance_info".ref_chunk_id
	// WHERE "chunk".updated=0
	virtual auto get_not_updated_instance(void) -> gaenari::dataset::dataframe = 0;
	// WHERE "chunk".id=?
	virtual void get_instance_by_chunk_id(_in int64_t chunk_id, _in callback_query cb) = 0;

	// get chunk_ids.
	// SELECT id FROM "chunk" WHERE updated=?
	virtual auto get_chunk(_in bool updated) -> std::vector<int64_t> = 0;

	// get check emptiness of generation table.
	// SELECT CASE WHEN EXISTS(SELECT 1 FROM generation) THEN 0 ELSE 1 END AS emptiness
	virtual bool get_is_generation_empty(void) = 0;

	// add generation and return id.
	// INSERT INTO "generation" (datetime) VALUES (?) RETURNING id
	virtual int64_t add_generation(_in int64_t datetime) = 0;

	// update generation.
	// UPDATE "generation" SET root_ref_treenode_id=? WHERE id=?
	virtual void update_generation(_in int64_t generation_id, _in int64_t root_ref_treenode_id) = 0;

	// update generation etc.
	// UPDATE "generation" SET instance_count=?, weak_instance_count=?, weak_instance_ratio=?, before_weak_instance_accuracy=?,
	//						   after_weak_instance_accuracy=?, before_instance_accuracy=?, after_instance_accuracy WHERE id=?
	virtual void update_generation_etc(_in int64_t generation_id, _in int64_t instance_count, _in int64_t weak_instance_count,
					_in double weak_instance_ratio, _in double before_weak_instance_accuracy, _in double after_weak_instance_accuracy,
					_in double before_instance_accuracy, _in double after_instance_accuracy) = 0;

	// returns the last id value of the treenode.
	// SELECT id FROM "treenode" WHERE id=(SELECT MAX(id) FROM "treenode")
	virtual int64_t get_treenode_last_id(void) = 0;

	// add rule and return id.
	// (this is low-level and there is another high-level helper function.)
	// INSERT INTO "rule" (feature_index, rule_type, value_type, value_integer, value_real) VALUES (?,?,?,?,?) RETURNING id
	// value_type : 0 (value_int), 1(value_double).
	virtual int64_t add_rule(_in int feature_index, _in int rule_type, _in int value_type, _in int64_t value_int, _in double value_double) = 0;

	// add leaf info and return id.
	// (this is low-level and there is another high-level helper function.)
	// INSERT INTO "leaf_info" (label_index, type, go_to_ref_generation_id, correct_count, total_count, accuracy) VALUES (?,?,?,?,?,?) RETURNING id
	virtual int64_t add_leaf_info(_in int label_index, _in type::leaf_info_type type, _in int64_t go_to_ref_generation_id, _in int64_t correct_count, _in int64_t total_count, _in double accuracy) = 0;

	// add treenode and return id.
	// INSERT INTO "treenode" (ref_generation_id, ref_parent_treenode_id, ref_rule_id, ref_leaf_info_id) VALUES (?,?,?) RETURNING id
	// remark)
	// -1 has a special meaning. that is, it should not return -1.
	virtual int64_t add_treenode(_in int64_t ref_generation_id, _in int64_t ref_parent_treenode_id, _in int64_t ref_rule_id, _in int64_t ref_leaf_info_id) = 0;

	// get first root_ref_treenode_id.
	// SELECT root_ref_treenode_id FROM generation ORDER BY id ASC LIMIT 1
	virtual int64_t get_first_root_ref_treenode_id(void) = 0;

	// get treenodes with parent treenode id filter.
	// SELECT
	//		"${treenode}"."id"						AS "treenode.id"
	//		"${treenode}"."ref_generation_id"		AS "treenode.ref_generation_id"
	//		"${treenode}"."ref_parent_treenode_id"	AS "treenode.ref_parent_treenode_id"
	//		"${treenode}"."ref_rule_id"				AS "treenode.ref_rule_id"
	//		"${treenode}"."ref_leaf_info_id"		AS "treenode.ref_leaf_info_id"
	//		"${rule}"."feature_index"				AS "rule.feature_index"
	//		"${rule}"."rule_type"					AS "rule.rule_type"
	//		"${rule}"."value_type"					AS "rule.value_type"
	//		"${rule}"."value_integer"				AS "rule.value_integer"
	//		"${rule}"."value_real"					AS "rule.value_real"
	//		"${leaf_info}"."label_index"			AS "leaf_info.label_index"
	//		"${leaf_info}"."correct_count"			AS "leaf_info.correct_count"
	//		"${leaf_info}"."total_count"			AS "leaf_info.total_count"
	// FROM treenode
	//		LEFT JOIN rule      ON treenode.ref_rule_id = rule.id
	//		LEFT JOIN leaf_info ON treenode.ref_leaf_info_id = leaf_info.id
	// WHERE treenode.ref_parent_treenode_id = ?
	// (left join is used because treenode.ref_leaf_info_id = -1 is possible(-1 means that it's not leaf node).
	//  in the case of inner joins, nothing returned on treenode.ref_leaf_info_id = -1.)
	//	ex)	--------------------------------------------------------------------
	//		treenode.id	...	treenode.ref_leaf_info_id	...	leaf_info.id	...
	//		--------------------------------------------------------------------
	//		2			...	-1							...	NULL			NULL
	//		5			...	 3							...	3				...
	//		(treenode.id = 2) is not included when using inner joins.
	//		using a left join will return NULL even at (treenode.id = 2).
	virtual auto get_treenode(_in int64_t parent_treenode_id) -> std::vector<type::treenode_db> = 0;

	// update instance info.
	// UPDATE ${instance_info} SET ref_leaf_treenode_id=?, correct=? WHERE ref_instance_id=?
	virtual void update_instance_info(_in int64_t instance_id, _in int64_t ref_leaf_treenode_id, _in bool correct) = 0;

	// update chunk.
	// UPDATE chunk SET updated=?, initial_correct_count=?, total_count=?, initial_accuracy=? WHERE id=?
	virtual void update_chunk(_in int64_t chunk_id, _in bool updated, _in int64_t initial_correct_count, _in int64_t total_count, _in double initial_accuracy) = 0;

	// update chunk total_count.
	// UPDATE chunk SET total_count=? WHERE id=?
	virtual void update_chunk_total_count(_in int64_t chunk_id, _in int64_t total_count) = 0;
	
	// get chunk list.
	// SELECT * FROM chunk ORDER BY datetime ASC
	virtual void get_chunk_list(_in callback_query cb) = 0;

	// get chunk_updated.
	// SELECT updated FROM chunk WHERE id=?
	virtual bool get_chunk_updated(_in int64_t chunk_id) = 0;

	// update leaf_info.
	// UPDATE leaf_info 
	// SET correct_count=correct_count+?,
	//	   total_count=total_count+?,
	//	   accuracy=(CASE WHEN total_count+?=0 THEN 0.0 ELSE (CASE(correct_count AS REAL)+?)/(total_count+?) END)
	// WHERE=?
	virtual void update_leaf_info(_in int64_t leaf_info_id, _in int64_t increment_correct_count, _in int64_t increment_total_count) = 0;

	// get_root_ref_treenode_id.
	// SELECT root_ref_treenode_id FROM generation WHERE id=?
	virtual int64_t get_root_ref_treenode_id(_in int64_t generation_id) = 0;

	// get weak treenode.
	// SELECT treenode.id AS "treenode.id" FROM leaf_info
	// INNER JOIN treenode ON treenode.ref_leaf_info_id = leaf_info.id
	// WHERE leaf_info.accuracy <= ? AND leaf_info.total_count >= ? AND leaf_info.type = 1
	// remark 1)
	// - leaf_info.type = 1(=leaf_info_type::leaf)
	// - leaf_info.type is not included in the index,
	//   but seach combined with AND is optimized.
	//   (https://www.sqlite.org/queryplanner.html)
	// remark 2)
	// - modify with update_leaf_info_by_go_to_generation_id.
	virtual auto get_weak_treenode(_in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound) -> std::vector<int64_t> = 0;

	// update leaf_info go to generation.
	// UPDATE leaf_info SET type=2, go_to_ref_generation_id=?
	// WHERE leaf_info.accuracy <= ? AND leaf_info.total_count >= ? AND leaf_info.type = 1
	// remark 1)
	// - leaf_info.type = 1(=leaf_info_type::leaf)
	// - leaf_info.type = 2(=leaf_info_type::go_to_generation)
	// - leaf_info.type is not included in the index,
	//   but seach combined with AND is optimized.
	//   (https://www.sqlite.org/queryplanner.html)
	// remark 2)
	// - if the structure(query logic) changes,
	//   supul_t::model::update_leaf_info_by_go_to_generation_id_to_cache(...)
	//   must also be modified.
	virtual void update_leaf_info_by_go_to_generation_id(_in int64_t generation_id, _in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound) = 0;

	// get instance by go_to_generation_id.
	// SELECT "instance".*
	// FROM "instance"
  	//		INNER JOIN "instance_info"	ON "instance_info".ref_instance_id	= "instance".id
  	//		INNER JOIN "treenode"		ON "treenode".id					= "instance_info".ref_leaf_treenode_id
  	//		INNER JOIN "leaf_info"		ON "leaf_info".id					= "treenode".ref_leaf_info_id
	// WHERE "leaf_info".go_to_ref_generation_id=?
	virtual auto get_instance_by_go_to_generation_id(_in int64_t go_to_ref_generation_id) -> gaenari::dataset::dataframe = 0;

	// get_correct_instance_count_by_go_to_generation_id.
	// SELECT COUNT(*) FROM "instance"
	//		INNER JOIN "instance_info"	ON "instance_info".ref_instance_id	= "instance".id
	//		INNER JOIN "treenode"		ON "treenode".id					= "instance_info".ref_leaf_treenode_id
	//		INNER JOIN "leaf_info"		ON "leaf_info".id					= "treenode".ref_leaf_info_id
	// WHERE "leaf_info".go_to_ref_generation_id=? AND "instance_info".correct = 1
	virtual auto get_correct_instance_count_by_go_to_generation_id(_in int64_t go_to_ref_generation_id) -> int64_t = 0;

	// update instance_info_with_weak_count_increment.
	// UPDATE instance_info SET ref_leaf_treenode_id=?, correct=?, weak_count = weak_count + 1 WHERE ref_instance_id=?
	virtual void update_instance_info_with_weak_count_increment(_in int64_t instance_id, _in int64_t ref_leaf_treenode_id, _in bool correct) = 0;

	// copy rule.
	// INSERT INTO rule (feature_index, rule_type, ...)
	// SELECT feature_index, rule_type, ...
	// FROM rule
	// WHERE id=?
	// RETURNING id
	virtual int64_t copy_rule(_in int64_t src_rule_id) = 0;

	// update rule (value_integer).
	// UPDATE rule SET value_integer=? WHERE id=?
	virtual void update_rule_value_integer(_in int64_t rule_id, _in int64_t value_integer) = 0;

	// get generation_id_by_treenode_id.
	// SELECT ref_generation_id FROM treenode WHERE id=?
	virtual int64_t get_generation_id_by_treenode_id(_in int64_t treenode_id) = 0;

	// get leaf_info by chunk_id.
	// SELECT instance."%y" as "instance.actual",
	//		  instance_info.weak_count as "instance_info.weak_count", 
	//		  leaf_info.*
	// FROM instance_info
	//		INNER JOIN instance ON instance_info.id = instance.id
	//		INNER JOIN treenode ON instance_info.ref_leaf_treenode_id = treenode.id
	//		INNER JOIN leaf_info ON treenode.ref_leaf_info_id = leaf_info.id
	// WHERE instance_info.ref_chunk_id = ?
	virtual void get_leaf_info_by_chunk_id(_in int64_t chunk_id, _in callback_query cb) = 0;

	// get chunk total_count by chunk_id.
	// SELECT total_count FROM chunk WHERE id=?
	virtual int64_t get_total_count_by_chunk_id(_in int64_t chunk_id) = 0;

	// delete instance_by_chunk_id.
	// DELETE FROM instance
	// WHERE id IN(
	// 		SELECT instance_info.id
	// 		FROM instance_info
	// 		INNER JOIN chunk ON instance_info.ref_chunk_id = chunk.id
	// 		WHERE chunk.id = ?
	// )
	virtual void delete_instance_by_chunk_id(_in int64_t chunk_id) = 0;

	// delete instance_info_by_chunk_id.
	// DELETE FROM instance_info
	// WHERE id IN(
	// 		SELECT instance_info.id
	// 		FROM instance_info
	// 		INNER JOIN chunk ON instance_info.ref_chunk_id = chunk.id
	// 		WHERE chunk.id = ?
	// )
	virtual void delete_instance_info_by_chunk_id(_in int64_t chunk_id) = 0;

	// delete chunk_by_id.
	// DELETE FROM chunk WHERE id=?
	virtual void delete_chunk_by_id(_in int64_t chunk_id) = 0;

	// get global_row_count.
	// SELECT COUNT(*) FROM global
	virtual int64_t get_global_row_count(void) = 0;

	// add global_one_row.
	// INSERT INTO ${global} (id) VALUES (1)
	virtual void add_global_one_row(void) = 0;

	// get global variables.
	// SELECT * FROM global WHERE id=1
	virtual auto get_global(void) -> type::map_variant = 0;

	// set global variable(s).
	// increment = false
	//		UPDATE global SET key=?, [...] WHERE id = 1
	// increment = true
	//		UPDATE global SET key=key+?, [...] WHERE id = 1
	// remark 1) `g` can be a subset of the whole global variables.
	//			 that is, only b, c values can be set when the entire global has a, b, c.
	// remark 2) it is difficult to implement as a static prepared statement.
	// remark 3) see sqlite::set_global().
	virtual void set_global(_in const type::map_variant& g, _option_in bool increment = false) = 0;

	// verification (for `tests`).
	// no need to optimize.
	// 1. SELECT COUNT(*) FROM instance_info
	// 2. SELECT COUNT(*) FROM instance_info WHERE correct=1
	// 3. SELECT COUNT(*) FROM instance 
	//		INNER JOIN instance_info ON instance.id = instance_info.ref_instance_id
	//		INNER JOIN chunk ON chunk.id = instance_info.ref_chunk_id
	//    WHERE chunk.updated=1
	// 4. SELECT SUM(leaf_info.total_count) FROM treenode
	//		INNER JOIN leaf_info on leaf_info.id = treenode.ref_leaf_info_id
	//    WHERE leaf_info.type = 1
	// 5. SELECT SUM(weak_count) FROM instance_info
	// 6. SELECT instance."%y%" AS actual, leaf_info.label_index AS predicted FROM instance
	//		INNER JOIN instance_info ON instance_info.ref_instance_id = instance.id
	//		INNER JOIN treenode ON treenode.id = instance_info.ref_leaf_treenode_id
	//		INNER JOIN leaf_info ON leaf_info.id = treenode.ref_leaf_info_id
	virtual int64_t get_instance_count(void) = 0;
	virtual int64_t get_instance_correct_count(void) = 0;
	virtual int64_t get_updated_instance_count(void) = 0;
	virtual int64_t get_sum_leaf_info_total_count(void) = 0;
	virtual int64_t get_sum_weak_count(void) = 0;
	virtual void get_instance_actual_predicted(_in callback_query cb) = 0;

	// global confusion matrix.
	// 1. SELECT * FROM global_confusion_matrix;
	// 2. SELECT COUNT(id) FROM global_confusion_matrix WHERE actual=? AND predicted=?;
	// 3. INSERT INTO global_confusion_matrix (actual, predicted) VALUES (?, ?);
	// 4. UPDATE global_confusion_matrix SET count = coalesce(count, 0) + ? WHERE actual = ? AND predicted = ? RETURNING id
	// remark)
	// - update_global_confusion_matrix_item_increment(...) returns bool.
	//   returns true if an actual or predicted condition exists, false otherwise.
	//   `RETURNING id` is for checking if the condition is true, so it doesn't return the id value.
	virtual auto    get_global_confusion_matrix(void) -> std::vector<type::map_variant> = 0;
	virtual int64_t get_global_confusion_matrix_item_count(_in int64_t actual, _in int64_t predicted) = 0;
	virtual void    add_global_confusion_matrix_item(_in int64_t actual, _in int64_t predicted) = 0;
	virtual bool    update_global_confusion_matrix_item_increment(_in int64_t actual, _in int64_t predicted, _in int64_t increment) = 0;

	// for `tests`.
	// no need to optimize.
	// get chunk_intial_accuracy.
	// SELECT initial_accuracy FROM chunk
	virtual auto get_chunk_initial_accuracy(void) -> std::vector<double> = 0;
	// SELECT id FROM chunk ORDER BY id DESC LIMIT 1
	virtual int64_t get_chunk_last_id(void) = 0;
	// SELECT * FROM chunk WHERE id = ?
	virtual auto get_chunk_by_id(_in int64_t id) -> type::map_variant = 0;

	// for `report`.
	// SELECT * FROM chunk WHERE updated = 1
	virtual auto get_chunk_for_report(void) -> std::unordered_map<std::string, std::vector<type::value_variant>> = 0;
	// SELECT * FROM generation
	virtual auto get_generation_for_report(void) -> std::unordered_map<std::string, std::vector<type::value_variant>> = 0;

	// helper.
public:
	int64_t add_rule(_in const gaenari::method::decision_tree::rule_t& rule);
	int64_t add_leaf_info(_in const gaenari::method::decision_tree::leaf_info_t& leaf_info);

	// short-cut.
public:
	auto get_confusion_matrix_by_all_instances(void) -> std::unordered_map<int, std::unordered_map<int, int64_t>>;
};

// add_rule(feature_index, rule_type, ...) is the low-level for database interface.
// application call this function.
inline int64_t base::add_rule(_in const gaenari::method::decision_tree::rule_t& rule) {
	// default value.
	int64_t value_int = -1;
	double value_double = -1;

	// get value type (0 : int, 1 : double)
	int value_type = 0;
	auto& arg = rule.args[0];
	auto& type = arg.valueype;
	switch (type) {
	case gaenari::type::value_type::value_type_double:
		value_type = 1;
		value_double = arg.numeric_double;
		break;
	case gaenari::type::value_type::value_type_int:
		value_int = static_cast<int64_t>(arg.numeric_int32);
		break;
	case gaenari::type::value_type::value_type_int64:
		value_int = arg.numeric_int64;
		break;
	case gaenari::type::value_type::value_type_size_t:
		value_int = static_cast<int64_t>(arg.index);
		break;
	default:
		THROW_SUPUL_INTERNAL_ERROR0;
	}

	// call low-level.
	return add_rule(static_cast<int>(rule.feature_indexes[0]), 
					static_cast<int>(rule.type),
					value_type, 
					value_int, 
					value_double);
}

// add_leaf_info(label_index, ...) is the low-level for database interface.
// application call this function.
inline int64_t base::add_leaf_info(_in const gaenari::method::decision_tree::leaf_info_t& leaf_info) {
	int64_t total = static_cast<int64_t>(leaf_info.correct_count + leaf_info.incorrect_count);
	if (total == 0) THROW_SUPUL_INTERNAL_ERROR0;
	double accuracy = static_cast<double>(leaf_info.correct_count) / static_cast<double>(total);
	return add_leaf_info(static_cast<int>(leaf_info.label_string_index),
						 type::leaf_info_type::leaf,
						 -1,
						 static_cast<int64_t>(leaf_info.correct_count), 
						 total, accuracy);
}

inline auto base::get_confusion_matrix_by_all_instances(void) -> std::unordered_map<int, std::unordered_map<int, int64_t>> {
	std::unordered_map<int, std::unordered_map<int, int64_t>> ret;
	// scan all instances.
	get_instance_actual_predicted([&ret](auto& row) -> bool {
		auto actual    = common::get_variant_int(row, "actual");
		auto predicted = common::get_variant_int(row, "predicted");
		ret[actual][predicted]++;
		return true;
	});
	return ret;
}

} // db
} // supul

#endif // HEADER_GAENARI_SUPUL_DB_BASE_HPP
