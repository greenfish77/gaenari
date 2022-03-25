#ifndef HEADER_GAENARI_SUPUL_SUPUL_SUPUL_HPP
#define HEADER_GAENARI_SUPUL_SUPUL_SUPUL_HPP

// supul - increment learning for decision tree.
//
// required
//   - base directory  : project data files are saved.
//   - property.txt    : project configuration file. see comments in file.
//   - attributes.json : define the data field names and types such as REAL and INTEGER.
//                       and specify the items of X and Y to be used for learning.
//                       (note: X items can only be added, Y item can not be changed.)
//
// supul api.
//
//   - supul.api.init(base_dir)
//     . project base directory required.
//       if the directory exists, create a new project. if not, load it.
//       if a new project is created, edit the file below.
//       - property.txt    : project configuration file. see comments in file.
//       - attributes.json : define the data field names and types such as REAL and INTEGER.
//                           and specify the items of X and Y to be used for learning.
//                           (note: X items can only be aded, Y item can not be changed.)
//     . initialize supul and database connection.
//
//   - supul.api.insert_chunk_csv(csv_file_path)
//     . `chunk` is one dataset with labeled(=Y) instances.
//     . save the `chunk` to supul database. that is, all `chunks` are stored in the database.
//     . it only saves to the database and does not affect the model.
//
//   - supul.api.update()
//     . evaluates unevaluated(=not updated) instances so that weak instances can be found.
//     . update only statistical informations and does not affect the model.
//     . if the tree has not been trained yet, the first tree is trained.  
//
//   - supul.api.rebuild()
//     . it finds weak instances among the stored full instances.
//       after re-trains them, entire forest will grow.
//     . as the partial trees are modified, so this affects the model.
//
//   - supul.api.predict((name, value) map)
//     . return predict result with one unlabeled instance row.

namespace supul {
namespace supul {

class supul_t {
public:
	supul_t(): string_table{*this}, model{*this}, api{*this} {}
	~supul_t() { deinit(); }

	// supul main public api.
protected:
	static void create_project(_in const std::string& base_dir);
	void init(_in const std::string& base_dir);
	void deinit(void);
	void on_first_initialized(void);

	// supul apis are in categories.
	// so, it can be used like this:
	// supul.<category>.api(...).

	// model api category.
public:
	class model {
	public:
		// treenode cache size: capacity(1G), survive(0.5G)
		model(supul_t& supul): supul{supul}, treenode_cache{1'073'741'824, 536'870'912}, get_root_ref_treenode_id_cache{4096, 1280} {}
		~model() { deinit(); }

	public:
		void init(void);
		void deinit(void);

		// main functions of the model
		//   - execute the transaction.
	public:
		void insert_chunk_csv(_in const std::string& csv_file_path);
		void update(void);
		void rebuild(void);
		auto predict(_in const type::map_variant& x) -> type::treenode_db;

		// other functions of the model.
		//   - call in the transaction.
		void verify_all(void);
		void remove_chunk(_in size_t chunk_id);

		// internal functions.
	protected:
		db::base& get_db(void);
		void build_first_tree(_out int64_t& instance_count);
		int64_t insert_tree(_in const gaenari::method::decision_tree::decision_tree& dt, _option_in int64_t* generation_id = nullptr, _option_out std::unordered_map<int, int64_t>* treenode_id_map = nullptr);
		bool eval_treenode(_in const type::treenode_db& treenode, _in const type::map_variant& x) const;
		bool is_correct(_in const type::map_variant& instance, _in const type::treenode_db& leaf_treenode, _option_out int* label_index = nullptr, _option_out int* predicted_label_index = nullptr) const;
		auto predict_main(_in const type::map_variant& x) -> type::predict_info;
		auto get_weak_treenode_condition(void);
		auto get_treenode_from_cache(_in int64_t parent_treenode_id) -> const std::vector<type::treenode_db>;
		void update_leaf_info_to_cache(_in int64_t leaf_info_id, _in int64_t increment_correct_count, _in int64_t increment_total_count);
		void update_leaf_info_by_go_to_generation_id_to_cache(_in int64_t generation_id, _in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound);
		int64_t get_root_ref_treenode_id_from_cache(_in int64_t generation_id);
		type::treenode_db& get_leaf_treenode_with_rule_add(_in _out type::predict_info& predict_info, _in const type::map_variant& instance, _out bool& added);
		void update_generation_etc(_in int64_t generation_id, _in const type::map_variant& before_global, _in const int64_t weak_instance_count, _in const int64_t before_weak_instance_correct_count, _in const int64_t after_weak_instance_correct_count);
		void chunk_limit(_in int64_t lower_bound, _in int64_t upper_bound);

	protected:
		supul_t& supul;
		gaenari::dataset::feature_select fs;
		std::optional<int64_t> first_root_ref_treenode_id;
		bool initialized = false;
		friend class supul_tester;	// for tester.

		// caches.
		// modify clear_all_cache() on add.
	protected:
		void clear_all_cache(void);
		void verify_cache(void);
		void verify_global(void);
		void verify_etc(void);
		gaenari::common::cache<int64_t, std::vector<type::treenode_db>> treenode_cache;
		gaenari::common::cache<int64_t, int64_t> get_root_ref_treenode_id_cache;
	};

	// string table api category.
public:
	class string_table {
	public:
		string_table(supul_t& supul): supul{supul} {}
		~string_table() { deinit(); }

	public:
		void init(void);
		void deinit(void);

	public:
		void load(void);
		int  add(_in const std::string& s);
		const gaenari::common::string_table& get_table(void) const;
		void flush(void);

	protected:
		supul_t& supul;
		gaenari::common::string_table strings;
	};

	// report api category.
public:
	class report {
	public:
		report(supul_t& supul): supul{supul} {}

	public:
		struct result {
			std::optional<std::string> json;
			std::optional<std::string> plot_png_path;
		};

		enum class plot_option {no, show, save_png, show_with_save_png};

	public:
		auto json(_in const std::string& option_as_json) -> std::string;
		static auto gnuplot(_in const std::string& json, _in const std::map<std::string, std::string>& options) -> std::string;

	protected:
		static auto global_for_gnuplot(_in const gaenari::common::json_insert_order_map& j) -> type::map_variant;
		static void chunk_history_for_gnuplot(_in const gaenari::common::json_insert_order_map& j, _out common::gnuplot::vnumbers& chunk_histories, _out std::vector<std::string>& chunk_datetime);
		static void generation_history_for_gnuplot(_in const gaenari::common::json_insert_order_map& j, _out common::gnuplot::vnumbers& generation_histories, _out std::vector<std::string>& generation_datetime);
		static std::string global_confusion_matrix_data_block_for_gnuplot(_in const gaenari::common::json_insert_order_map& json, _in const std::string& data_block_name, _out std::vector<std::string>& label_names, _out int64_t& max_count);

	protected:
		void chunk_history_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json);
		void global_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json);
		void confusion_matrix_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json);
		void generation_history_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json);

	protected:
		supul_t& supul;
		const std::vector<std::string> supported_categories = {"global", "confusion_matrix", "generation_history", "chunk_history"};
	};

	// public api catagory.
	// the only functions accessible from outside.
public:
	class api {
	public:
		api(supul_t& supul): supul{supul}, lifetime{*this}, project{*this}, model{*this}, report{*this}, misc{*this}, property{*this}, test{*this} {}
		~api() { lifetime.close(); }

	protected:
		// base class.
		struct base { base(supul_t::api& api): api{api}{} protected: supul_t::api& api; };

	public:
		// life time.
		struct lifetime: public base {
			bool open(_in const std::string& base_dir) noexcept;
			void close(void) noexcept;
		} lifetime;
	
		// project.
		// static functions, so just call supul::supul::supul_t::api::*(...).
		struct project: public base {
			static bool create(_in const std::string& base_dir) noexcept;
			static bool set_property(_in const std::string& base_dir, _in const std::string& name, _in const std::string& value) noexcept;
			static bool add_field(_in const std::string& base_dir, _in const std::string& name, _in const std::string& dtype) noexcept;
			static bool x(_in const std::string& base_dir, _in const std::vector<std::string>& x) noexcept;
			static bool y(_in const std::string& base_dir, _in const std::string& y) noexcept;
		} project;

		// modeling.
		struct model: public base {
			bool insert_chunk_csv(_in const std::string& csv_file_path) noexcept;
			bool update(void) noexcept;
			bool rebuild(void) noexcept;
			auto predict(_in const std::unordered_map<std::string, std::string>& x) noexcept -> type::predict_result;
		} model;

		// report.
		struct report: public base {
			auto json(_in const std::string& option_as_json) noexcept -> std::string;
			static auto gnuplot(_in const std::string& json, _in const std::map<std::string, std::string>& options) noexcept -> std::optional<std::string>;
		} report;

		// misc.
		struct misc: public base {
			static auto version(void) noexcept -> const std::string&;
			auto errmsg(void) const noexcept -> std::string;
		} misc;

		// property.
		struct property: public base {
			bool set_property(_in const std::string& name, _in const std::string& value) noexcept;
			auto get_property(_in const std::string& name, _in const std::string& def) noexcept -> std::string;
			bool save(void) noexcept;
			bool reload(void) noexcept;
		} property;

		// tests.
		struct test: public base {
			bool verify(void) noexcept;
		} test;

	protected:
		supul_t& supul;
		std::string errormsg;
		bool initialized = false;
		friend class supul_tester;	// for tester.
	};

protected:
	// for test.
	friend class supul_tester;

	// common shared data.
	type::paths paths;
	gaenari::common::prop prop;
	type::attributes attributes;

	// db schema.
	db::schema schema;

	// db object.
	db::base* db = nullptr;

	// api categories.
	string_table string_table;
	model model;

	// supul exports only `api` category.
public:
	api api;
};

} // supul
} // supul

// footer include for implementation.
#include "impl/supul.hpp"
#include "impl/model.hpp"
#include "impl/string_table.hpp"
#include "impl/api.hpp"
#include "impl/report.hpp"

#endif // HEADER_GAENARI_SUPUL_SUPUL_SUPUL_HPP
