#ifndef HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE3_HPP
#define HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE3_HPP

namespace supul {
namespace db {
namespace sqlite {

class sqlite_t: public base {
public:
	sqlite_t() = delete;
	sqlite_t(const db::schema& schema): base{schema} {}
	virtual ~sqlite_t() { deinit(); }

	// db base api.
public:
	virtual void init(_in const gaenari::common::prop& property, _in const type::paths& paths);
	virtual void deinit(void);

	virtual void begin(_in bool exclusive);
	virtual void commit(void);
	virtual void rollback(void);
	virtual void lock_timeout(_in int msec);

	virtual int	    count_string_table(void);
	virtual int     get_string_table_last_id(void);
	virtual void    get_string_table(_in callback_query cb);
	virtual void    add_string_table(_in int id, _in const std::string& text);
	virtual int64_t add_chunk(_in int64_t datetime);
	virtual int64_t add_instance(_in const type::vector_variant& params);
	virtual void    add_instance_info(_in int64_t instance_id, _in int64_t chunk_id);
	virtual auto    get_not_updated_instance(void) -> gaenari::dataset::dataframe;
	virtual void    get_instance_by_chunk_id(_in int64_t chunk_id, _in callback_query cb);
	virtual auto    get_chunk(_in bool updated) -> std::vector<int64_t>;
	virtual bool    get_is_generation_empty(void);
	virtual int64_t add_generation(_in int64_t datetime);
	virtual void    update_generation(_in int64_t generation_id, _in int64_t root_ref_treenode_id);
	virtual void    update_generation_etc(_in int64_t generation_id, _in int64_t instance_count, _in int64_t weak_instance_count,
		_in double weak_instance_ratio, _in double before_weak_instance_accuracy, _in double after_weak_instance_accuracy,
		_in double before_instance_accuracy, _in double after_instance_accuracy);
	virtual int64_t get_treenode_last_id(void);
	virtual int64_t add_rule(_in int feature_index, _in int rule_type, _in int value_type, _in int64_t value_int, _in double value_double);
	virtual int64_t add_leaf_info(_in int label_index, _in type::leaf_info_type type, _in int64_t go_to_ref_generation_id, _in int64_t correct_count, _in int64_t total_count, _in double accuracy);
	virtual int64_t add_treenode(_in int64_t ref_generation_id, _in int64_t ref_parent_treenode_id, _in int64_t ref_rule_id, _in int64_t ref_leaf_info_id);
	virtual int64_t get_first_root_ref_treenode_id(void);
	virtual auto    get_treenode(_in int64_t parent_treenode_id) -> std::vector<type::treenode_db>;
	virtual void    update_instance_info(_in int64_t instance_id, _in int64_t ref_leaf_treenode_id, _in bool correct);
	virtual void    update_chunk(_in int64_t chunk_id, _in bool updated, _in int64_t initial_correct_count, _in int64_t total_count, _in double initial_accuracy);
	virtual void	update_chunk_total_count(_in int64_t chunk_id, _in int64_t total_count);
	virtual void	get_chunk_list(_in callback_query cb);
	virtual bool	get_chunk_updated(_in int64_t chunk_id);
	virtual void    update_leaf_info(_in int64_t leaf_info_id, _in int64_t increment_correct_count, _in int64_t increment_total_count);
	virtual auto    get_weak_treenode(_in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound) -> std::vector<int64_t>;
	virtual void    update_leaf_info_by_go_to_generation_id(_in int64_t generation_id, _in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound);
	virtual auto    get_instance_by_go_to_generation_id(_in int64_t go_to_ref_generation_id) -> gaenari::dataset::dataframe;
	virtual auto    get_correct_instance_count_by_go_to_generation_id(_in int64_t go_to_ref_generation_id) -> int64_t;
	virtual void    update_instance_info_with_weak_count_increment(_in int64_t instance_id, _in int64_t ref_leaf_treenode_id, _in bool correct);
	virtual int64_t get_root_ref_treenode_id(_in int64_t generation_id);
	virtual int64_t copy_rule(_in int64_t src_rule_id);
	virtual void    update_rule_value_integer(_in int64_t rule_id, _in int64_t value_integer);
	virtual int64_t get_generation_id_by_treenode_id(_in int64_t treenode_id);
	virtual void	get_leaf_info_by_chunk_id(_in int64_t chunk_id, _in callback_query cb);
	virtual int64_t	get_total_count_by_chunk_id(_in int64_t chunk_id);
	virtual void	delete_instance_by_chunk_id(_in int64_t chunk_id);
	virtual void	delete_instance_info_by_chunk_id(_in int64_t chunk_id);
	virtual void	delete_chunk_by_id(_in int64_t chunk_id);
	virtual int64_t get_global_row_count(void);
	virtual void    add_global_one_row(void);
	virtual auto    get_global(void) -> type::map_variant;
	virtual void    set_global(_in const type::map_variant& g, _option_in bool increment = false);
	virtual int64_t get_instance_count(void);
	virtual int64_t get_instance_correct_count(void);
	virtual int64_t get_updated_instance_count(void);
	virtual int64_t get_sum_leaf_info_total_count(void);
	virtual int64_t get_sum_weak_count(void);
	virtual void    get_instance_actual_predicted(_in callback_query cb);

	virtual auto    get_global_confusion_matrix(void) -> std::vector<type::map_variant>;
	virtual int64_t get_global_confusion_matrix_item_count(_in int64_t actual, _in int64_t predicted);
	virtual void    add_global_confusion_matrix_item(_in int64_t actual, _in int64_t predicted);
	virtual bool    update_global_confusion_matrix_item_increment(_in int64_t actual, _in int64_t predicted, _in int64_t increment);

	virtual auto    get_chunk_initial_accuracy(void) -> std::vector<double>;
	virtual int64_t get_chunk_last_id(void);
	virtual auto    get_chunk_by_id(_in int64_t id) -> type::map_variant;

	virtual auto	get_chunk_for_report(void) -> std::unordered_map<std::string, std::vector<type::value_variant>>;
	virtual auto	get_generation_for_report(void) -> std::unordered_map<std::string, std::vector<type::value_variant>>;

	// sqlite engine.
protected:
	// sqlite sql execute engine.
	void execute(_in stmt stmt_type, _in const type::vector_variant& params, _in callback_query cb);
	auto execute(_in stmt stmt_type, _in const type::vector_variant& params, _option_in bool error_on_multiple_row) -> type::map_variant;
	void execute(_in stmt stmt_type, _in const type::vector_variant& params, _out std::vector<type::map_variant>& results, _option_in size_t max_row_counts = 1024);
	auto execute(_in stmt stmt_type, _in const type::vector_variant& params) -> gaenari::dataset::dataframe;
	auto execute_column_based(_in stmt stmt_type, _in const type::vector_variant& params) -> std::unordered_map<std::string, std::vector<type::value_variant>>;

	// only one column is returned as a vector of a specific name and type.
	template <typename T>
	auto execute(_in stmt stmt_type, _in const type::vector_variant& params, _in const std::string& name) -> std::vector<T>;

	// sqlite utils.
protected:
	// create table.
	void create_table_if_not_existed(_in const db::schema& schema);

	// create index.
	void create_index_if_not_existed(_in const db::schema& schema);

	// stmt pool.
	void build_stmt_pool(void);
	sqlite3_stmt* get_stmt(_in const std::string& sql);
	stmt_info& find_stmt(_in stmt stmt_type);
	void clear_stmt_pool(void);

	// etc.
	std::string error_desc(_in const std::optional<int> error_code={}) const;

	// dataframe repository and utils.
	friend class repository;
protected:
	std::vector<gaenari::dataset::usecols> get_usecols(_in const type::fields& fields) const;

protected:
	// sqlite3 object.
	sqlite3* db = nullptr;

	// stmt pool.
	std::unordered_map<stmt, stmt_info> stmt_pool;
};

} // sqlite
} // db
} // gaenari

#include "impl/sqlite.repository.hpp"
#include "impl/sqlite.engine.hpp"
#include "impl/sqlite.impl.hpp"

#endif // HEADER_GAENARI_SUPUL_DB_SQLITE_SQLITE3_HPP
