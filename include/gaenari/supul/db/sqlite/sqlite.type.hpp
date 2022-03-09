#ifndef HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_TYPE_HPP
#define HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_TYPE_HPP

namespace supul {
namespace db {
namespace sqlite {

// include sqlite3.h inside the namespace.
#include "sqlite/sqlite3.h"

// query type.
enum class stmt {
	unknown,
	count_string_table,
	get_string_table_last_id,
	get_string_table,
	add_string_table,
	add_chunk,
	add_instance,
	add_instance_info,
	get_not_updated_instance,
	get_chunk,
	get_is_generation_empty,
	add_generation,
	update_generation,
	update_generation_etc,
	get_treenode_last_id,
	add_rule,
	add_leaf_node,
	add_treenode,
	get_instance_by_chunk_id,
	get_first_root_ref_treenode_id,
	get_treenode,
	update_instance_info,
	update_chunk,
	update_leaf_info,
	get_weak_treenode,
	update_leaf_info_by_go_to_generation_id,
	get_instance_by_go_to_generation_id,
	get_correct_instance_count_by_go_to_generation_id,
	update_instance_info_with_weak_count_increment,
	get_root_ref_treenode_id,
	copy_rule,
	update_rule_value_integer,
	get_generation_id_by_treenode_id,
	get_global_row_count,
	add_global_one_row,
	get_global,
	get_instance_count,
	get_instance_correct_count,
	get_updated_instance_count,
	get_sum_leaf_info_total_count,
	get_sum_weak_count,
	get_instance_actual_predicted,
	get_global_confusion_matrix,
	get_global_confusion_matrix_item_count,
	add_global_confusion_matrix_item,
	update_global_confusion_matrix_item_increment,

	get_chunk_initial_accuracy,
	get_chunk_last_id,
	get_chunk_by_id,

	get_chunk_for_report,
	get_generation_for_report,
};

// implement the prepared statement to satisfy performance and security.
// define statement information.
//  - stmt   : sqlite statement object
//  - fields : fields of the query result.
struct stmt_info {
	sqlite3_stmt* stmt = nullptr;
	type::fields fields;
	stmt_info(_in sqlite3_stmt* stmt, _in const type::fields& fields): stmt{stmt}, fields{fields} {}
};

} // sqlite
} // db
} // gaenari

#endif // HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE_TYPE_HPP
