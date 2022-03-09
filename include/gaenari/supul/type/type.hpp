#ifndef HEADER_GAENARI_SUPUL_TYPE_TYPE_HPP
#define HEADER_GAENARI_SUPUL_TYPE_TYPE_HPP

namespace supul {
namespace type {

struct paths {
	std::string base_dir;
	std::string conf_dir;
	std::string sqlite_dir;
	std::string property_txt;
	std::string attributes_json;
};

enum class field_type {
	UNKNOWN  = 0,  // db type  | c++ type    | note
				   // ---------+-------------+-----
	INTEGER  = 10, // INTEGER  | int64_t     |
	BIGINT   = 11, // BIGINT   | int64_t     |
	SMALLINT = 12, // SMALLINT | int64_t     |
	TINYINT  = 13, // TINYINT  | int64_t     |
	TEXT_ID  = 20, // INTEGER  | int64_t     | ref. string table id
	REAL     = 30, // REAL     | double      |
	TEXT     = 40, // TEXT     | std::string |
};

// database field type can be null, int64, double, or string.
using value_variant = std::variant<std::monostate, int64_t, double, std::string>;

// field is (name, type) map.
using fields = gaenari::common::insert_order_map<std::string, field_type>;

// from attributes.json
struct attributes {
	int revision = 0;
	type::fields fields;
	std::vector<std::string> x;
	std::string y;
	void clear() {
		*this = attributes();
	}
};

// define the index of the table.
// multiple columns index is not yet supported.
// check whether the index is used using the command below.
// EXPLAIN QUERY PLAN <query>.
struct index {
	std::string column_name;
};

// informations of one database table.
// table name prefix is not added. to get real name, use schema.get_real_table_name(...).
struct table_info {
	std::string			name;
	type::fields		fields;
	std::vector<index>	indexes;
};

// the enum value is used as the array index,
// so it is designed to increment by 1.
// the order can be changed, but the last `table_count` must match.
enum class table {
	instance				= 0,	// attributes.json.
	instance_info			= 1,
	chunk					= 2,
	generation				= 3,
	treenode				= 4,
	rule					= 5,
	leaf_info				= 6,
	string_table			= 7,
	global					= 8,
	global_confusion_matrix	= 9,

	// count.
	table_count				= 10,
};

// rebuild reason.
enum class rebuild_reason {
	unnecessary,	// do not call rebuild.
	empty_treenode,	// since no tree has been learned yet, it is forced to rebuild.
};

// leaf_info::type.
// delared as TINYINT so defined up to 255.
enum class leaf_info_type {
	unknown				= 0,
	leaf				= 1,
	go_to_generation	= 2,
};

// treenode used by db.
// it is separated from the treenode used in gaenari.
struct treenode_db {
	int64_t id = 0;
	struct rule_t {
		// remark)
		// feature_index is the order of attributes.json.
		// if `id` is included in row or instance, +1.
		int64_t			id						= 0;
		int				feature_index			= 0; // check remark) +1 or +0.
		int				rule_type				= 0;
		int				value_type				= 0;
		int64_t			value_integer			= 0;
		double			value_real				= 0.0;
	} rule;
	bool is_leaf_node = false;
	struct leaf_info_t {
		int64_t			id						= 0;
		int				label_index				= 0;
		leaf_info_type	type					= leaf_info_type::unknown;
		int64_t			go_to_ref_generation_id	= -1;
		int64_t			correct_count			= 0;
		int64_t			total_count				= 0;
		double			accuracy				= 0.0;
	} leaf_info;

	void clear(void) {
		*this = treenode_db();
	}

	inline bool operator==(const treenode_db& s) const {
		if (id != s.id) return false;
		if (rule.id != s.rule.id) return false;
		if (rule.feature_index != s.rule.feature_index) return false;
		if (rule.rule_type != s.rule.rule_type) return false;
		if (rule.value_type != s.rule.value_type) return false;
		if (rule.value_integer != s.rule.value_integer) return false;
		if (rule.value_real != s.rule.value_real) return false;
		if (is_leaf_node != s.is_leaf_node) return false;
		if (leaf_info.id != s.leaf_info.id) return false;
		if (leaf_info.label_index != s.leaf_info.label_index) return false;
		if (leaf_info.type != s.leaf_info.type) return false;
		if (leaf_info.go_to_ref_generation_id != s.leaf_info.go_to_ref_generation_id) return false;
		if (leaf_info.correct_count != s.leaf_info.correct_count) return false;
		if (leaf_info.total_count != s.leaf_info.total_count) return false;
		if (leaf_info.accuracy != s.leaf_info.accuracy) return false;
		return true;
	}

	inline bool operator!=(const treenode_db& s) const {
		return not (*this == s);
	}
};

// predict result status.
// - leaf_node
//   apply rules up to leaf nodes (normal case).
// - middle_node
//   in case of not matching leaf node, but matching in the middle of generation.
// - not found
//   does not match. has a new type of nominal value been entered?
enum class predict_status {
	unknown     = 0,
	leaf_node   = 1,
	middle_node = 2,
	not_found   = 3,
};

struct predict_info {
	predict_status status = predict_status::unknown;
	treenode_db leaf_treenode;						// predict_status::leaf_node
	treenode_db middle_treenode;					// predict_status::middle_node

	// no nominal value found in child rules of treenode_id below.
	int64_t parent_treenode_id_leaf_not_found = 0;	// predict_status::middle_node, predict_status::not_found.

	// new rule built about no nominal value?
	// and its treenode value.
	bool new_rule_built = false;
	treenode_db new_treenode;
};

// (name, variant) insert_order_map.
using map_variant = gaenari::common::insert_order_map<std::string, type::value_variant>;

// (name, variant) unordered_map.
using unordered_map_variant = std::unordered_map<std::string, type::value_variant>;

// variant vector.
using vector_variant = std::vector<type::value_variant>;

// predict result.
struct predict_result {
	bool		error = false;
	std::string	errormsg;
	int64_t		label_index = 0;
	std::string	label;
	int64_t		correct_count = 0;
	int64_t		total_count = 0;
	double		accuracy = 0.0;
	void clear(void) {
		*this = predict_result();
	}
};

} // type
} // supul

#endif // HEADER_GAENARI_SUPUL_TYPE_TYPE_HPP
