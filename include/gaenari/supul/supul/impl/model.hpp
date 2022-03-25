#ifndef HEADER_GAENARI_SUPUL_SUPUL_IMPL_MODEL_HPP
#define HEADER_GAENARI_SUPUL_SUPUL_IMPL_MODEL_HPP

// footer included from supul.hpp
namespace supul {
namespace supul {

inline void supul_t::model::init(void) {
	// get the feature selection for the dataset.
	fs.x_add(supul.attributes.x, false);
	fs.y_set(supul.attributes.y, false);

	// get global.
	auto global = get_db().get_global();
	auto db_schema_version   = common::get_variant_int64(global, "schema_version");
	auto code_schema_version = supul.schema.version();
	if (db_schema_version != code_schema_version) THROW_SUPUL_ERROR(common::f("schema version mis-match: %0 != %1.", {db_schema_version, code_schema_version}));
	gaenari::logger::info("schema version: {0}", {code_schema_version});
	gaenari::logger::info("model initialized.");

	// set initialized.
	initialized = true;
}

inline void supul_t::model::deinit(void) {
#ifndef NDEBUG
	verify_all();
#endif
	initialized = false;
}

inline db::base& supul_t::model::get_db(void) {
	if (not supul.db) THROW_SUPUL_ERROR("database is not initialized.");
	return *supul.db;
}

inline void supul_t::model::insert_chunk_csv(_in const std::string& csv_file_path) {
	int row_count = 0;
	gaenari::dataset::csv_reader csv;
	type::vector_variant params;
	std::vector<size_t> indexes;

	// transaction begin.
	db::transaction_guard transaction{get_db(), true};

	// add chunk and get id.
	int64_t datetime = gaenari::common::current_yyyymmddhhmmss();
	int64_t chunk_id = get_db().add_chunk(datetime);
	gaenari::logger::info("new chunk added, id: {0}", {chunk_id});

	// get instance table info.
	// and, get the field names and types without `id`.
	// `params` is column data to put in one instance row, and reserve the size.
	auto& instance_table = supul.schema.get_table_info(type::table::instance);
	auto  [names, types] = common::fields_to_vector(instance_table.fields, {"id"});
	size_t param_count = names.size();
	params.resize(param_count);

	// open csv and get header map(name, index).
	if (not csv.open(csv_file_path, ',', true)) THROW_SUPUL_ERROR(common::f("fail to open csv file: %0.", {csv_file_path}));
	gaenari::logger::info("{0} is opened.", {csv_file_path});
	auto& csv_header_map = csv.get_header_map_ref();

	// since the order of table field names and csv header names may be difference,
	// manage the indexes.
	// if equals, set to {0, 1, 2, ...}.
	for (const auto& name: names) {
		auto find = csv_header_map.find(name);
		if (find == csv_header_map.end()) THROW_SUPUL_ERROR(common::f("not found: %0", {name.get()}));
		indexes.push_back(find->second);
	}

	// traverse csv rows.
	// it can be accessed as `csv[fieldname]`,
	// but for performance reasons, it is access as a vector with `indexes`.
	gaenari::logger::info("start to save csv rows to db.");
	for (;;) {
		if (not csv.move_next_row(nullptr)) break;
		auto& csv_row = csv.get_row_data_ref();
		for (size_t i=0; i<param_count; i++) {
			auto& type  = types[i];			// type::field_type.
			auto& index = indexes[i];
			auto& value = csv_row[index];	// std::string.

			// csv row items to params.
			if ((type == type::field_type::INTEGER) or
				(type == type::field_type::BIGINT)  or
				(type == type::field_type::SMALLINT)) {
				params[i] = static_cast<int64_t>(std::stoll(value));
			} else if (type == type::field_type::TEXT_ID) {
				params[i] = static_cast<int64_t>(supul.string_table.add(value));
			} else if (type == type::field_type::REAL) {
				params[i] = std::stod(value);
			} else if (type == type::field_type::TEXT) {
				params[i] = value;
			}
		}

		// db action.
		// csv row to db row.
		int64_t instance_id = get_db().add_instance(params);

		// add instance info.
		get_db().add_instance_info(instance_id, chunk_id);

		row_count++;
	}

	// flush string table.
	supul.string_table.flush();

	// close csv.
	csv.close();

	// update total count.
	get_db().update_chunk_total_count(chunk_id, row_count);

	// set global.
	get_db().set_global({{"instance_count", static_cast<int64_t>(row_count)}}, true);

	// auto limit chunk?
	supul.prop.reload();
	auto use = supul.prop.get("limit.chunk.use", false);
	if (use) {
		auto lower_bound = supul.prop.get("limit.chunk.instance_lower_bound", 0LL);
		auto upper_bound = supul.prop.get("limit.chunk.instance_upper_bound", 0LL);
		chunk_limit(lower_bound, upper_bound);
	}

	// transaction commit.
	transaction.commit();
	gaenari::logger::info("completed to save({0} row(s)).", {row_count});

	// success.
	return;
}

inline void supul_t::model::update(void) {
	bool build_first_tree_completed = false;
	int64_t increment_total_count = 0;
	int64_t increment_total_correct_count = 0;
	int64_t instance_count_on_first_tree = 0;
	std::unordered_map<int, std::unordered_map<int, int64_t>> confusion_matrix;	// [actual][predicted] = count.

	// (leaf_info.id, (increment correct_count, increment total_count)) map.
	std::unordered_map<int64_t, std::pair<int64_t, int64_t>> increment_count;

	// transaction begin.
	db::transaction_guard transaction{get_db(), true};

	// get not updated chunk ids.
	auto not_updated_chunk_ids = get_db().get_chunk(false);
	if (not_updated_chunk_ids.empty()) {
		// all chunks are updated.
		gaenari::logger::info("all chunks are already updated.");
		return;
	}
	gaenari::logger::info("not updated chunks(id={0}) found.", {gaenari::common::vec_to_string(not_updated_chunk_ids)});

	// is `generation` empty?
	if (get_db().get_is_generation_empty()) {
		// build first tree.
		build_first_tree(instance_count_on_first_tree);
		build_first_tree_completed = true;
	}

	// get instances that have not yet been updated with a query.
	// callback, not dataframe.
	for (auto& chunk_id: not_updated_chunk_ids) {
		int64_t correct_count = 0;
		int64_t total_count = 0;

		// instances loop in chunk.
		get_db().get_instance_by_chunk_id(chunk_id, [&supul=supul, &increment_count, &correct_count, &total_count, &confusion_matrix](auto& row) -> bool {
			// predict instance and get its leaf node.
			auto instance_id = common::get_variant_int64(row, "id");
			bool middle_leaf_by_not_matched = false;
			bool new_leaf_node_added = false;
			type::predict_status status = type::predict_status::unknown;
			auto  predict_info = supul.model.predict_main(row);

			// when nominal value that is not included in training, the leaf_treenode cannot be found.
			// the function below adds a new tree node so that it can return a leaf node anyway.
			auto& leaf_treenode = supul.model.get_leaf_treenode_with_rule_add(predict_info, row, new_leaf_node_added);
			if (not leaf_treenode.is_leaf_node) THROW_SUPUL_INTERNAL_ERROR0;

			// correct?
			int actual = 0;
			int predicted = 0;
			auto correct = supul.model.is_correct(row, leaf_treenode, &actual, &predicted);
			confusion_matrix[actual][predicted]++;

			// update instance_info (leaf_treenode_id, correct).
			supul.db->update_instance_info(instance_id, leaf_treenode.id, correct);

			// calc increment count.
			if (correct) {
				correct_count++;
				total_count++;
				if (not new_leaf_node_added) {
					increment_count[leaf_treenode.leaf_info.id].first++;	// add correct count.
					increment_count[leaf_treenode.leaf_info.id].second++;	// add total count.
				}
			} else {
				total_count++;
				if (not new_leaf_node_added) {
					increment_count[leaf_treenode.leaf_info.id].second++;	// add total count only.
				}
			}

			return true;
		});

		// accuracy.
		double accuracy = 0.0;
		if (total_count == 0) {
			// something wierd?
			gaenari::logger::warn("empty chunk.");
		} else {
			accuracy = static_cast<double>(correct_count) / static_cast<double>(total_count);
		}

		// this chunk has been updated.
		get_db().update_chunk(chunk_id, true, correct_count, total_count, accuracy);
		gaenari::logger::info("chunk(id={0}) updated.", {chunk_id});

		// set increment values.
		increment_total_count += total_count;
		increment_total_correct_count += correct_count;
	}

	// update leaf_info count.
	// if build_first_tree_completed, count is already processed.
	if (not build_first_tree_completed) {
		for (const auto& it: increment_count) {
			const auto& leaf_info_id			= it.first;
			const auto& increment_correct_count	= it.second.first;
			const auto& increment_total_count	= it.second.second;

			// update.
			get_db().update_leaf_info(leaf_info_id, increment_correct_count, increment_total_count);
			update_leaf_info_to_cache(leaf_info_id, increment_correct_count, increment_total_count);
		}
	}

	// increment global updated_instance_count.
	if (build_first_tree_completed) {
		// already called in build_first_tree().
		// do validation.
		if (increment_total_count != instance_count_on_first_tree) {
			THROW_SUPUL_ERROR(common::f("build_first_tree() condition failed: %0 != %1", {increment_total_count, instance_count_on_first_tree}));
		}
	} else {
		get_db().set_global({{"updated_instance_count", increment_total_count}}, true);
	}

	// update global variables.
	get_db().set_global({{"instance_correct_count", increment_total_correct_count}}, true);
	auto g = get_db().get_global();
	double instance_accuracy = static_cast<double>(common::get_variant_int64(g, "instance_correct_count")) / static_cast<double>(common::get_variant_int64(g, "updated_instance_count"));
	get_db().set_global({{"instance_accuracy", instance_accuracy}});

	// update confusion matrix.
	for (const auto& it: confusion_matrix) {
		auto actual = static_cast<int64_t>(it.first);
		for (const auto& it2: it.second) {
			auto  predicted = static_cast<int64_t>(it2.first);
			auto& count = it2.second;
			if (not get_db().update_global_confusion_matrix_item_increment(actual, predicted, count)) {
				// insert confusion matrix item.
				get_db().add_global_confusion_matrix_item(actual, predicted);
				if (not get_db().update_global_confusion_matrix_item_increment(actual, predicted, count))
					THROW_SUPUL_ERROR("fail to update_global_confusion_matrix_item_increment.");
			}
		}
	}

	// transaction commit.
	transaction.commit();
	gaenari::logger::info("update completed.");

	// success.
	return;
}

// get leaf node from predict_info.
// if leaf node is not found, build a new treenode by adding rule.
// it is mutable operation, so do not call from predict().
// predict() is immutable operation with no exclusive lock.
// so, it is only called from update().
inline type::treenode_db& supul_t::model::get_leaf_treenode_with_rule_add(_in _out type::predict_info& predict_info, _in const type::map_variant& instance, _out bool& added) {
	type::treenode_db ret;

	// clear output.
	added = false;

	if (predict_info.status == type::predict_status::leaf_node) {
		// matched to leaf node (mostly).
		return predict_info.leaf_treenode;
	}

	if ((predict_info.status == type::predict_status::middle_node) or
		(predict_info.status == type::predict_status::not_found)) {
		// not found leaf node.
		// instance contains a nominal value that did not exist during training.

		// called twice? not yet defined.
		if (predict_info.new_rule_built) THROW_SUPUL_INTERNAL_ERROR0;

		// get parent treenode id.
		auto& parent_treenode_id = predict_info.parent_treenode_id_leaf_not_found;

		// it's not a terminal leaf node, it's an intermediate-generation leaf node.
		// get childs, and choose first one.
		auto childs = get_treenode_from_cache(parent_treenode_id);
		if (childs.empty()) THROW_SUPUL_INTERNAL_ERROR0;
		auto& child = childs[0];

		// get nominal feature name of the rule.
		//supul.schema.get_tables(type::table::instance)
		if (child.rule.feature_index >= supul.attributes.x.size()) THROW_SUPUL_INTERNAL_ERROR0;
		auto& feature_name = supul.attributes.x[child.rule.feature_index];

		// check rule: the type of feature name must be nominal.
		auto& fields = supul.schema.get_table_info(type::table::instance).fields;
		auto find = fields.find(feature_name);
		if (find == fields.end()) THROW_SUPUL_INTERNAL_ERROR0;
		if (find->second != type::field_type::TEXT_ID) THROW_SUPUL_INTERNAL_ERROR0;

		// check rule: <nominal type feature name> = <value>.
		if (child.rule.rule_type != static_cast<int>(gaenari::method::decision_tree::rule_t::rule_type::cmp_equ)) THROW_SUPUL_INTERNAL_ERROR0;

		// get a nominal value that did not exist before.
		auto new_nominal_value = common::get_variant_int64(instance, feature_name);

		// get a ground truth of instance.
		// i can't believe this label, but i have nothing to rely on yet.
		auto label = common::get_variant_int64(instance, supul.attributes.y);

		// we add a new rule as `if <nominal type feature name> = <not found new nominal value> => then => class = label`.
		// copies one of the children.
		// and chage the value.
		auto new_rule_id  = get_db().copy_rule(child.id);
		get_db().update_rule_value_integer(new_rule_id, new_nominal_value);

		// get generation id.
		auto generation_id = get_db().get_generation_id_by_treenode_id(child.id);

		// add a new leaf info.
		auto new_leaf_info_id = get_db().add_leaf_info(	static_cast<int>(label),	// label_index.
														type::leaf_info_type::leaf, // type. (as leaf node)
														-1,							// not go to generation.
														1,							// correct_count.
														1,							// total_count.
														1.0);						// accuracy.

		// finally, add treenode.
		auto new_treenode_id = get_db().add_treenode(generation_id, parent_treenode_id, new_rule_id, new_leaf_info_id);
																
		// set cache.
		// in fact, only the cache can be updated,
		// but it is read from the database after erase.
		// there is a performance loss, but it's a clearer way.
		treenode_cache.erase(parent_treenode_id);
		auto new_childs = get_treenode_from_cache(parent_treenode_id);

		// one item added.
		if (childs.size() + 1 != new_childs.size()) THROW_SUPUL_INTERNAL_ERROR0;

		for (auto& child: new_childs) {
			if (child.id == new_treenode_id) {
				// new rule built.
				predict_info.new_rule_built = true;
				predict_info.new_treenode = child;

				// some validation.
				if ((child.rule.id != new_rule_id) or
					(child.rule.value_integer != new_nominal_value)) {
					THROW_SUPUL_INTERNAL_ERROR0;
				}

				break;
			}
		}

		if (not predict_info.new_rule_built) {
			// we did `get_db().add_treenode(...)`.
			// but not found `supul.model.get_treenode_from_cache(...)`.
			THROW_SUPUL_INTERNAL_ERROR0;
		}

		gaenari::logger::warn("new treenode built: id={0}. (feature_name: {1}, value: {2})", {new_treenode_id, feature_name, new_nominal_value});

		// leaf node added.
		added = true;

		// return added one.
		return predict_info.new_treenode;
	}

	THROW_SUPUL_INTERNAL_ERROR0;
}

// declare before rebuild() for auto return.
inline auto supul_t::model::get_weak_treenode_condition(void) {
	struct ret_t {
		double  accuracy	= 0.0;
		int64_t total_count	= 0;
	} ret;

	// reload property.
	if (not supul.prop.reload()) THROW_SUPUL_ERROR("fail to reload property.");

	// get value.
	ret.accuracy    = supul.prop.get("model.weak_treenode_condition.accuracy", -1.1);
	ret.total_count = supul.prop.get("model.weak_treenode_condition.total_count", 0LL);
	if ((ret.accuracy < 0) or (ret.total_count == 0)) THROW_SUPUL_ERROR("invalid weak_treenode_condition.");

	return ret;
}

inline void supul_t::model::rebuild(void) {
	int64_t increment_correct_count = 0;
	std::unordered_map<int, std::unordered_map<int, int64_t>> before_confusion_matrix;	// [actual][predicted] = count.
	std::unordered_map<int, std::unordered_map<int, int64_t>> after_confusion_matrix;	// [actual][predicted] = count.

	// transaction begin.
	db::transaction_guard transaction{get_db(), true};

	// get condition.
	auto condition = get_weak_treenode_condition();
	gaenari::logger::info("model.weak_treenode_condition.accuracy: {0}.", {condition.accuracy});
	gaenari::logger::info("model.weak_treenode_condition.total_count: {0}.", {condition.total_count});

	// get weaks.
	auto weak_treenode_ids = get_db().get_weak_treenode(condition.accuracy, condition.total_count);

	// no weaks, exit.
	if (weak_treenode_ids.empty()) {
		transaction.rollback();
		gaenari::logger::info("no weak treenodes found.");
		return;
	} else {
		gaenari::logger::info("weak treenodes found, ({0}).", {gaenari::common::vec_to_string(weak_treenode_ids)});
	}

	// get global for etc data of generation.
	auto before_global = get_db().get_global();

	// add generation.
	auto datetime = gaenari::common::current_yyyymmddhhmmss();
	auto generation_id = get_db().add_generation(datetime);
	gaenari::logger::info("new generation added. (id={0}, datetime={1})", {generation_id, datetime});

	// update go_to_generation.
	// change the go_to_generation value of the treenode that satisfies the weak condition.
	get_db().update_leaf_info_by_go_to_generation_id(generation_id, condition.accuracy, condition.total_count);
	update_leaf_info_by_go_to_generation_id_to_cache(generation_id, condition.accuracy, condition.total_count);

	// get instances.
	auto df = get_db().get_instance_by_go_to_generation_id(generation_id);
	df.set_string_table_reference_from(supul.string_table.get_table());
	gaenari::method::stringfy::logger(df, "dataframe to rebuild.", 20);
	if (df.empty()) THROW_SUPUL_INTERNAL_ERROR0;

	// correct count before rebuild.
	auto before_weak_instance_correct_count = get_db().get_correct_instance_count_by_go_to_generation_id(generation_id);
	gaenari::logger::info("before weak correct instance count: {0} / {1}.", {before_weak_instance_correct_count, df.rows()});

	// dataframe to dataset.
	gaenari::dataset::dataset ds(df, fs);
	gaenari::method::decision_tree::decision_tree dt;

	// train.
	gaenari::common::elapsed_time elapsed;
	gaenari::logger::info("start to train.");
	dt.train(ds);
	gaenari::logger::info("finished, elapsed: {0}", {elapsed.to_string()});

	// check empty.
	if (dt.empty()) {
		gaenari::logger::warn("trained, but empty. rebuild failed.");
		transaction.rollback();
		// we can either call update_leaf_info_by_go_to_generation_id_to_cache()
		// after this part or just clear the treenode cache,
		// but simply clear the whole thing here.
		clear_all_cache();
		return;
	}

	// print tree.
	auto stringfy = dt.stringfy("text/colortag", true);
	gaenari::logger::info(std::string("\n") + stringfy, true);

	// confusion matrix.
	double after_weak_instance_accuracy = 0.0;
	int64_t after_weak_instance_correct_count = 0;
	std::map<size_t,std::map<size_t,size_t>> confusion;
	dt.eval(df, after_weak_instance_accuracy, &confusion, nullptr, (size_t*)(&after_weak_instance_correct_count));
	auto cm = dt.to_string_confusion_matrix(confusion);
	gaenari::logger::info(std::string("\n") + cm, true);

	// did the rebuild improve performance?
	if (before_weak_instance_correct_count >= after_weak_instance_correct_count) {
		gaenari::logger::warn("no rebuild effect, weak_instances: {0}, before_weak_instance_correct_count: {1} -> after_weak_instance_correct_count: {2}.", {df.rows(), before_weak_instance_correct_count, after_weak_instance_correct_count});
		gaenari::logger::info("do rollback.");
		transaction.rollback();
		clear_all_cache();
		return;
	}

	// insert tree to db.
	std::unordered_map<int, int64_t> treenode_id_map;
	insert_tree(dt, &generation_id, &treenode_id_map);

	// the leaf tree nodes of rebuilt instances can change.
	// so, predict instances each row in dataframe,
	// and update with the changed value(leaf node).
	// prediction is possible with model(database) or gaenari(memory).
	// choose gaenari(memory). it's fast.

	// find `id` column index of dataframe.
	auto find = df.find_column_index("id", gaenari::dataset::data_type_t::data_type_int64);
	if (find == std::nullopt) THROW_SUPUL_INTERNAL_ERROR0;
	size_t id_col_index = find.value();

	// find `leaf_info.label_index` column index of dataframe.
	find = df.find_column_index("leaf_info.label_index", gaenari::dataset::data_type_t::data_type_int);
	if (find == std::nullopt) THROW_SUPUL_INTERNAL_ERROR0;
	size_t id_label_index = find.value();

	// every row in dataframe, do prediction.
	size_t row_count = df.rows();
	size_t row_index = 0;
	std::map<std::string, gaenari::type::variant> row_map;
	for (row_index=0; row_index<row_count; row_index++) {
		// get each row as (name, variant value) map.
		df.get_row_as_map(row_index, row_map, true, false);

		// instance id.
		int64_t instance_id = df.get_raw(row_index, id_col_index).numeric_int64;

		// before predicted.
		int label_index = df.get_raw(row_index, id_label_index).numeric_int32;

		// predict.
		int treenode_id = 0;
		auto label = dt.predict(row_map, &treenode_id);
		auto& ground_truth = ds.y.get_raw(row_index, 0);

		// get correct, treenode id(db).
		bool correct = false;
		if (label == ground_truth.index) correct = true;
		auto find = treenode_id_map.find(treenode_id);
		if (find == treenode_id_map.end()) THROW_SUPUL_INTERNAL_ERROR0;
		int64_t& db_treenode_id = find->second;

		// update.
		get_db().update_instance_info_with_weak_count_increment(instance_id, db_treenode_id, correct);

		// set confusion matrix.
		before_confusion_matrix[static_cast<int>(ground_truth.index)][label_index]++;
		after_confusion_matrix [static_cast<int>(ground_truth.index)][static_cast<int>(label)]++;
	}

	// update global variables.
	get_db().set_global({{"instance_correct_count", after_weak_instance_correct_count - before_weak_instance_correct_count},
						 {"acc_weak_instance_count",static_cast<int64_t>(row_count)}},
						true);
	auto g = get_db().get_global();
	double instance_accuracy = static_cast<double>(common::get_variant_int64(g, "instance_correct_count")) / static_cast<double>(common::get_variant_int64(g, "updated_instance_count"));
	get_db().set_global({{"instance_accuracy", instance_accuracy}});

	// update confusion matrix.
	auto confusion_matrix_diff = common::get_confusion_matrix_diff(before_confusion_matrix, after_confusion_matrix);
	for (const auto& it: confusion_matrix_diff) {
		auto& actual = it.first;
		for (const auto& it2: it.second) {
			auto& predicted = it2.first;
			auto& increment = it2.second;
			if (not get_db().update_global_confusion_matrix_item_increment(static_cast<int64_t>(actual), static_cast<int64_t>(predicted), increment)) {
				// insert confusion matrix item.
				get_db().add_global_confusion_matrix_item(actual, predicted);
				if (not get_db().update_global_confusion_matrix_item_increment(static_cast<int64_t>(actual), static_cast<int64_t>(predicted), increment))
					THROW_SUPUL_ERROR("fail to update_global_confusion_matrix_item_increment.");
			}
		}
	}

	// update generaton_etc.
	update_generation_etc(generation_id, before_global, static_cast<int64_t>(df.rows()), before_weak_instance_correct_count, after_weak_instance_correct_count);

	// transaction commit.
	transaction.commit();
	gaenari::logger::info("rebuild completed.");
}

inline void supul_t::model::build_first_tree(_out int64_t& instance_count) {
	int64_t root_ref_treenode_id = -1;
	std::unordered_map<int, int64_t> treenode_id_map; // treenode inner id(class treenode::id) -> treenode table id(database id).

	// clear output.
	instance_count = 0;

	// get not updated instances as dataframe.
	auto df = get_db().get_not_updated_instance();
	df.set_string_table_reference_from(supul.string_table.get_table());
	gaenari::method::stringfy::logger(df, "dataframe to build_first_tree.", 10);
	instance_count = static_cast<int64_t>(df.rows());

	// set updated_instance_count.
	// it has not been updated yet, but set in advance.
	// this is because the value is used in the insert_tree() below.
	// reason) stmt::update_generation
	//		   ... (SELECT CAST(? as REAL)/(SELECT updated_instance_count FROM ${global} ...
	//		   if updated_instance_count = 0, it's divide by zero.
	auto updated_instance_count = common::get_variant_int64(get_db().get_global(), "updated_instance_count");
	if (updated_instance_count != 0) THROW_SUPUL_ERROR(common::f("update_instance_count must be zero. value: %0", {updated_instance_count}));
	get_db().set_global({{"updated_instance_count", instance_count}});

	// dataframe to dataset.
	gaenari::dataset::dataset ds(df, fs);
	gaenari::method::decision_tree::decision_tree dt;

	// train.
	gaenari::common::elapsed_time elapsed;
	gaenari::logger::info("start to train.");
	dt.train(ds);
	gaenari::logger::info("finished, elapsed: {0}", {elapsed.to_string()});

	// print tree.
	auto stringfy = dt.stringfy("text/colortag", true);
	gaenari::logger::info(std::string("\n") + stringfy, true);

	// confusion matrix.
	double accuracy = 0.0;
	std::map<size_t,std::map<size_t,size_t>> confusion;
	dt.eval(df, accuracy, &confusion);
	auto cm = dt.to_string_confusion_matrix(confusion);
	gaenari::logger::info(std::string("\n") + cm, true);

	// insert tree to db.
	auto generation_id = insert_tree(dt, nullptr, &treenode_id_map);

	// update generation_etc.
	get_db().update_generation_etc(generation_id, instance_count, instance_count, 1.0, 0.0, accuracy, 0.0, accuracy);
}

// insert tree to db.
// return generation_id.
// treenode_id_map
//    treenode inner id(class treenode::id, always 0-based.)
//    ->
//    treenode table id(database id, unique guarantee for the entire database).
// if generation_id is not passed, it is added automatically.
inline int64_t supul_t::model::insert_tree(_in const gaenari::method::decision_tree::decision_tree& dt, _option_in int64_t* generation_id/*=nullptr*/, _option_out std::unordered_map<int, int64_t>* treenode_id_map/*=nullptr*/) {
	int64_t root_ref_treenode_id = -1;
	std::unordered_map<int, int64_t> _treenode_id_map; // treenode inner id(class treenode::id) -> treenode table id(database id).

	// add generation.
	int64_t _generation_id = 0;
	if (generation_id) {
		_generation_id = *generation_id;
	} else {
		auto datetime = gaenari::common::current_yyyymmddhhmmss();
		_generation_id = get_db().add_generation(datetime);
		gaenari::logger::info("new generation added. (id={0}, datetime={1})", {_generation_id, datetime});
	}

	// traverse tree.
	// `auto& treenode` improves readability, but uses the fullname for recognition of intellisense.
	gaenari::method::decision_tree::util::traverse_tree_node_const(dt.get_tree(), 
	[&supul=supul, _generation_id, &_treenode_id_map, &root_ref_treenode_id]
	(const gaenari::method::decision_tree::tree_node& treenode, int depth) -> bool {
		int64_t rule_id = 0;
		int64_t parent_treenode_id = 0;
		int64_t leaf_id = 0;
		int64_t treenode_id = 0;

		// rule, parent.
		if (not treenode.parent) {
			// if there is no parent, it is a root node.
			// a root node has no rules.
			rule_id = -1;
			parent_treenode_id = -1;
		} else {
			rule_id = supul.db->add_rule(treenode.rule);
			auto find = _treenode_id_map.find(treenode.parent->id);
			if (find == _treenode_id_map.end()) THROW_SUPUL_INTERNAL_ERROR0;
			parent_treenode_id = find->second;
		}

		// leaf.
		if (treenode.leaf) {
			leaf_id = supul.db->add_leaf_info(treenode.leaf_info);
		} else {
			leaf_id = -1;
		}

		// finally, add treenode.
		treenode_id = supul.db->add_treenode(_generation_id, parent_treenode_id, rule_id, leaf_id);
		if (treenode_id == -1) THROW_SUPUL_INTERNAL_ERROR0;

		// set (treenode::id, table id).
		_treenode_id_map[treenode.id] = treenode_id;

		// set root treenode id.
		if (treenode.id == 0) { 
			if (root_ref_treenode_id != -1) THROW_SUPUL_INTERNAL_ERROR0;	// twice called?
			root_ref_treenode_id = treenode_id;
		}

		return true;
	});

	// update generation.
	get_db().update_generation(_generation_id, root_ref_treenode_id);

	// copy treenode_id_map.
	if (treenode_id_map) *treenode_id_map = std::move(_treenode_id_map);

	return _generation_id;
}

// call db->get_treenode, but use cache to reduce db access.
// it does not return a reference. that is, the contents of the cache are copied.
// references may be returned, but dangling reference is an issue in future multi-threaded environments.
// just return the body without pre-locking.
inline auto supul_t::model::get_treenode_from_cache(_in int64_t parent_treenode_id) -> const std::vector<type::treenode_db> {
	// get value from cache.
	auto ret = treenode_cache.get(parent_treenode_id, [&supul=supul](_in auto& k, _out auto& v) {
		// not found in cache, get value from database.
		v = supul.db->get_treenode(k);
		v.shrink_to_fit();
	});

	return ret;
}

inline int64_t supul_t::model::get_root_ref_treenode_id_from_cache(_in int64_t generation_id) {
	auto ret = get_root_ref_treenode_id_cache.get(generation_id, [&supul=supul](_in auto& k, _out auto& v) {
		// not found in cache, get value from database.
		v = supul.db->get_root_ref_treenode_id(k);
	});
	return ret;
}

inline bool supul_t::model::eval_treenode(_in const type::treenode_db& treenode, _in const type::map_variant& x) const {
	bool ret = false;

	// get instance information.
	const auto& instance = supul.schema.get_table_info(type::table::instance);

	// feature_index is the location of attributes.json.
	// and instance.fields contains `id`.
	// so +1.
	// get fieldname and type of target rule.
	auto v = instance.fields.key(static_cast<size_t>(treenode.rule.feature_index) + 1);
	if (not v.has_value()) THROW_SUPUL_INTERNAL_ERROR0;
	auto& fieldname = v.value();
	auto f = instance.fields.find(fieldname);
	auto& fieldtype = f->second;

	// find value in x.
	auto find = x.find(fieldname);
	if (find == x.end()) THROW_SUPUL_ERROR1("field not found: %0.", fieldname);
	auto& x_value = find->second;
	auto index = x_value.index();

	// get rule type and value.
	auto rule_type = static_cast<gaenari::method::decision_tree::rule_t::rule_type>(treenode.rule.rule_type);
	bool is_double = (treenode.rule.value_type == 1);

	// must be int64_t or double.
	if (index == 3) THROW_SUPUL_INVALID_DATA_TYPE(fieldname);

	// compare.
	if (rule_type == gaenari::method::decision_tree::rule_t::rule_type::cmp_equ) {
		if (index == 1) {			// int64_t.
			if (is_double) THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
			else ret = std::get<1>(x_value) == treenode.rule.value_integer;
		} else if (index == 2) {	// double.
			if (is_double) ret = std::get<1>(x_value) == treenode.rule.value_real;
			else THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
		} else THROW_SUPUL_INTERNAL_ERROR0;
	} else if (rule_type == gaenari::method::decision_tree::rule_t::rule_type::cmp_lte) {
		if (index == 1) {			// int64_t.
			if (is_double) THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
			else ret = std::get<1>(x_value) <= treenode.rule.value_integer;
		} else if (index == 2) {	// double.
			if (is_double) ret = std::get<2>(x_value) <= treenode.rule.value_real;
			else THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
		} else THROW_SUPUL_INTERNAL_ERROR0;
	} else if (rule_type == gaenari::method::decision_tree::rule_t::rule_type::cmp_lt) {
		if (index == 1) {			// int64_t.
			if (is_double) THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
			else ret = std::get<1>(x_value) < treenode.rule.value_integer;
		} else if (index == 2) {	// double.
			if (is_double) ret = std::get<2>(x_value) < treenode.rule.value_real;
			else THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
		} else THROW_SUPUL_INTERNAL_ERROR0;
	} else if (rule_type == gaenari::method::decision_tree::rule_t::rule_type::cmp_gt) {
		if (index == 1) {			// int64_t.
			if (is_double) THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
			else ret = std::get<1>(x_value) > treenode.rule.value_integer;
		} else if (index == 2) {	// double.
			if (is_double) ret = std::get<2>(x_value) > treenode.rule.value_real;
			else THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
		} else THROW_SUPUL_INTERNAL_ERROR0;
	} else if (rule_type == gaenari::method::decision_tree::rule_t::rule_type::cmp_gte) {
		if (index == 1) {			// int64_t.
			if (is_double) THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
			else ret = std::get<1>(x_value) >= treenode.rule.value_integer;
		} else if (index == 2) {	// double.
			if (is_double) ret = std::get<2>(x_value) >= treenode.rule.value_real;
			else THROW_SUPUL_INVALID_DATA_TYPE(fieldname);
		} else THROW_SUPUL_INTERNAL_ERROR0;
	} else THROW_SUPUL_INTERNAL_ERROR0;

	return ret;
}

// returns a leaf treenode_db object reference with one x parameter row map(name, value) values.
// x is a (string, variant) map and the variant type must match the type specified in attributes.json.
inline auto supul_t::model::predict(_in const type::map_variant& x) -> type::treenode_db {
	// call predict_main with read-only transaction.
	// transaction begin.
	// implicit rollback called. its error could be ignored.
	db::transaction_guard transaction{get_db(), false};
	auto predict_info = predict_main(x);
	if (predict_info.status == type::predict_status::leaf_node) return predict_info.leaf_treenode;
	if (predict_info.status == type::predict_status::middle_node) return predict_info.middle_treenode;
	if (predict_info.status == type::predict_status::not_found) THROW_SUPUL_RULE_NOT_MATCHED_ERROR("no rule matched.");
	THROW_SUPUL_INTERNAL_ERROR0;
}

// predict main function.
// read-only database operation.
// maximize cache usage and minimize db access.
//
// returns treenode with the matched status.
// matching failure is due to a new nominal value or generation sampling.
// when the generation grows, learning can be performed with some nominal values rather than the whole.
// in this case, it may not be possible to match in future predict.
// backs up the matched go_to_generation treenode.
//
// ex) middle_node status.
// instance
// -----+----------+-----  .                                   -----+----------+-----  .
//  ... | category | y      |                                   ... | category | y      |
// -----+----------+-----   |                                  -----+----------+-----   |
//      |        a | ng      > tree built. -> ... -> sampling       |        t | ng      > new generation tree built.
//      |        b | ng     |  (rule: (a, b, ..., z))               |        m | ok     |  (rule: (t, m, p))
//      |      ... | ...    |     ^                                 |        p | ok     |
//      |        z | ok     |     |                            -----+----------+------ '
// -----+----------+-----  '      |                                       ^
//                                '-------------------- missing (n, e, w) | 
//                     find a match even in the middle.                   |
//                                                             -----+----------+-----
//                                                              ... | category | y
//                                                             -----+----------+-----
//                                                                  |        n | ?
//                                                                  |        e | ?
//                                                                  |        w | ?
//                                                             -----+----------+-----
//
// remark)
//   . the variant value type of map x must match attributes.json.
inline auto supul_t::model::predict_main(_in const type::map_variant& x) -> type::predict_info {
	// result.
	type::predict_info ret;
	std::optional<type::treenode_db> last_matched_treenode;

	// type checking.
	for (const auto& it: x) {
		const auto& name  = it.first;
		const auto  index = it.second.index();
		try {
			auto type = supul.schema.field_type(type::table::instance, name);
			switch (type) {
			case type::field_type::INTEGER:
			case type::field_type::BIGINT:
			case type::field_type::SMALLINT:
			case type::field_type::TINYINT:
			case type::field_type::TEXT_ID:
				if (index != 1) THROW_SUPUL_INVALID_DATA_TYPE(name);
				break;
			case type::field_type::REAL:
				if (index != 2) THROW_SUPUL_INVALID_DATA_TYPE(name);
				break;
			case type::field_type::TEXT:
				THROW_SUPUL_INVALID_DATA_TYPE(name);
				break;
			default:
				THROW_SUPUL_INTERNAL_ERROR1("invalid field type.");
			}
		} catch (const exceptions::item_not_found&) {
			continue;
		} catch (const exceptions::invalid_data_type&) {
			THROW_SUPUL_INVALID_DATA_TYPE(name);
		} catch(...) {
			THROW_SUPUL_ERROR1("fail to get field_type(%0).", name);
		}
	}

	// get root treenode id.
	int64_t cur_treenode_id = 0;
	if (this->first_root_ref_treenode_id) {
		// get from cache.
		cur_treenode_id = this->first_root_ref_treenode_id.value();
	} else {
		// read database, and cache.
		this->first_root_ref_treenode_id = supul.db->get_first_root_ref_treenode_id();
		cur_treenode_id = this->first_root_ref_treenode_id.value();
	}

	for (;;) {
		// get childs.
		// minimize db access by cache.
		auto childs = get_treenode_from_cache(cur_treenode_id);
		if (childs.empty()) {
			THROW_SUPUL_INTERNAL_ERROR0;
		}

		// does the variable x satisfy the condition?
		// get matched treenode in childs.
		std::optional<std::reference_wrapper<const type::treenode_db>> _matched_treenode;
		for (const auto& child: childs) {
			if (eval_treenode(child, x) == true) {
				_matched_treenode = child;
				break;
			}
		}

		if (not _matched_treenode.has_value()) {
			// oh, no matching.
			if (not last_matched_treenode.has_value()) {
				// entered a nominal value that has not been discovered yet?
				ret.status = type::predict_status::not_found;
				ret.parent_treenode_id_leaf_not_found = cur_treenode_id;
			} else {
				// not found, but found it in the middle of generation.
				ret.middle_treenode = last_matched_treenode.value();
				ret.status = type::predict_status::middle_node;
				ret.parent_treenode_id_leaf_not_found = cur_treenode_id;
			}

			// returns the last matched go_to_generation treenode.
			return ret;
		}

		auto& matched_treenode = _matched_treenode.value().get();

		// non-leaf node, go to matched child.
		if (not matched_treenode.is_leaf_node) {
			cur_treenode_id = matched_treenode.id;
			continue;
		}

		// leaf node, last choice or generation move.
		if (matched_treenode.leaf_info.type == type::leaf_info_type::leaf) {
			// found leaf node.
			// last choice.
			ret.status = type::predict_status::leaf_node;
			ret.leaf_treenode = matched_treenode;
			return ret;
		} else if (matched_treenode.leaf_info.type == type::leaf_info_type::go_to_generation) {
			last_matched_treenode = matched_treenode;

			// go to generation.
			// deep inside.
			auto go_to_treenode_id = get_root_ref_treenode_id_from_cache(matched_treenode.leaf_info.go_to_ref_generation_id);

			// jump.
			cur_treenode_id = go_to_treenode_id;
		} else {
			THROW_SUPUL_INTERNAL_ERROR0;
		}
	}
}

inline bool supul_t::model::is_correct(_in const type::map_variant& instance, _in const type::treenode_db& leaf_treenode, _option_out int* label_index, _option_out int* predicted_label_index) const {
	// get y fieldname.
	const auto& y = supul.attributes.y;

	// only leaf node.
	if (not leaf_treenode.is_leaf_node) THROW_SUPUL_INTERNAL_ERROR0;

	// get ground truth as label_index from instance.
	auto  _label_index = common::get_variant_int(instance, y);
	auto& _predicted_label_index = leaf_treenode.leaf_info.label_index;

	// set output.
	if (label_index) *label_index = _label_index;
	if (predicted_label_index) *predicted_label_index = _predicted_label_index;

	// compare!
	if (_label_index == _predicted_label_index) return true;
	return false;
}

inline void supul_t::model::update_leaf_info_to_cache(_in int64_t leaf_info_id, _in int64_t increment_correct_count, _in int64_t increment_total_count) {
	// lock cache.
	auto& mutex = treenode_cache.get_mutex();
	std::lock_guard<std::recursive_mutex> l(mutex);

	// get cache items.
	auto& cache_items = treenode_cache.get_items();

	for (auto& it: cache_items) {
		const int64_t& treenode_id = it.first;
		auto& treenode_dbs = it.second.first;
		for (auto& treenode: treenode_dbs) {
			if (treenode.leaf_info.id == leaf_info_id) {
				treenode.leaf_info.correct_count += increment_correct_count;
				treenode.leaf_info.total_count   += increment_total_count;
				if (treenode.leaf_info.total_count == 0) THROW_SUPUL_INTERNAL_ERROR0;
				treenode.leaf_info.accuracy       = static_cast<double>(treenode.leaf_info.correct_count) / static_cast<double>(treenode.leaf_info.total_count);
			}
		}
	}
}

inline void supul_t::model::update_leaf_info_by_go_to_generation_id_to_cache(_in int64_t generation_id, _in double leaf_node_accuracy_upperbound, _in int64_t leaf_node_total_count_lowerbound) {
	// lock cache.
	auto& mutex = treenode_cache.get_mutex();
	std::lock_guard<std::recursive_mutex> l(mutex);

	// get cache items.
	auto& cache_items = treenode_cache.get_items();

	// logically implement :
	// UPDATE leaf_info SET type=2, go_to_ref_generation_id=?
	// WHERE leaf_info.accuracy <= ? AND leaf_info.total_count >= ? AND leaf_info.type = 1
	for (auto& it: cache_items) {
		const int64_t& treenode_id = it.first;
		auto& treenode_dbs = it.second.first;
		for (auto& treenode: treenode_dbs) {
			if ((treenode.leaf_info.accuracy <= leaf_node_accuracy_upperbound) and
				(treenode.leaf_info.total_count >= leaf_node_total_count_lowerbound) and
				(treenode.leaf_info.type == type::leaf_info_type::leaf)) {
				treenode.leaf_info.type = type::leaf_info_type::go_to_generation;
				treenode.leaf_info.go_to_ref_generation_id = generation_id;
			}
		}
	}
}

inline void supul_t::model::clear_all_cache(void) {
	treenode_cache.clear();
	get_root_ref_treenode_id_cache.clear();
}

inline void supul_t::model::verify_cache(void) {
	if (not supul.db) return;

	// compare the cache contents with the database contents.

	// verify treenode_cache.
	{
		auto& mutex = treenode_cache.get_mutex();
		std::lock_guard<std::recursive_mutex> l(mutex);

		for (const auto& it: treenode_cache.get_items()) {
			const auto& parent_treenode_id = it.first;
			const auto& data_cache = it.second.first;
			const auto  data_db = get_db().get_treenode(parent_treenode_id);
			if (data_cache != data_db) THROW_SUPUL_INTERNAL_ERROR0;
		}
	}

	// verify get_root_ref_treenode_id_cache.
	{
		auto& mutex = get_root_ref_treenode_id_cache.get_mutex();
		std::lock_guard<std::recursive_mutex> l(mutex);

		for (const auto& it: get_root_ref_treenode_id_cache.get_items()) {
			const auto& generaton_id = it.first;
			const auto& data_cache = it.second.first;
			const auto  data_db = get_db().get_root_ref_treenode_id(generaton_id);
			if (data_cache != data_db) THROW_SUPUL_INTERNAL_ERROR0;
		}
	}

	gaenari::logger::info("verify_cache completed.");
}

inline void supul_t::model::verify_global(void) {
	auto g = get_db().get_global();
	auto s1 = common::get_variant_int64(g, "instance_count");
	auto s2 = common::get_variant_int64(g, "updated_instance_count");
	auto s3 = common::get_variant_int64(g, "instance_correct_count");
	auto t1 = get_db().get_instance_count();
	auto t2 = get_db().get_updated_instance_count();
	auto t3 = get_db().get_instance_correct_count();
	gaenari::logger::info("verify_global, instance_count: "			+ std::to_string(s1) + ',' + std::to_string(t1));
	gaenari::logger::info("verify_global, updated_instance_count: " + std::to_string(s2) + ',' + std::to_string(t2));
	gaenari::logger::info("verify_global, instance_correct_count: " + std::to_string(s3) + ',' + std::to_string(t3));
	if ((s1 != t1) or (s2 != t2) or (s3 != t3)) THROW_SUPUL_ERROR("fail to verify_global.");
}

inline void supul_t::model::verify_etc(void) {
	auto g = get_db().get_global();
	auto updated_instance_count = common::get_variant_int64(g, "updated_instance_count");
	auto acc_weak_instance_count = common::get_variant_int64(g, "acc_weak_instance_count");
	auto instance_correct_count = common::get_variant_int64(g, "instance_correct_count");

	// verify_etc_1 : verify get_sum_leaf_info_total_count == get_updated_instance_count.
	auto s1 = get_db().get_sum_leaf_info_total_count();
	auto u1 = get_db().get_updated_instance_count();
	if (s1 != u1) THROW_SUPUL_ERROR2("fail to verify_etc_1, sum_leaf_info_total_count(%0) != updated_instance_count(%1).", s1, u1);

	// verify_etc_2 : sum(weak_count) of instance == global.
	auto w1 = get_db().get_sum_weak_count();
	if (w1 != acc_weak_instance_count) THROW_SUPUL_ERROR2("fail to verify_etc_2, get_sum_weak_count(%0) != acc_weak_instance_count(%1).", w1, acc_weak_instance_count);

	// verify_etc_3 : check negative count in global confusion matrix.
	auto gcm = get_db().get_global_confusion_matrix();
	for (auto& it:gcm) if (common::get_variant_int64(it, "count") < 0) THROW_SUPUL_ERROR("fail to verify_etc_3.");

	// verify_etc_4 : sum(global_confusion_matrix::count) == global::updated_instance_count.
	int64_t sum_gcm = 0; for (auto& it:gcm) sum_gcm += common::get_variant_int64(it, "count");
	if (sum_gcm != updated_instance_count) THROW_SUPUL_ERROR2("fail to verify_etc_4, sum_gcm(%0) != updated_instance_count(%1).", sum_gcm, updated_instance_count);

	// verify_etc_5 : sum(correct count in confusion matrix) == global::instance_correct_count.
	int64_t sum_gcm_correct = 0;
	for (auto& it:gcm) {
		if (common::get_variant_int64(it, "actual") != common::get_variant_int64(it, "predicted")) continue;
		sum_gcm_correct += common::get_variant_int64(it, "count");
	}
	if (sum_gcm_correct != instance_correct_count) THROW_SUPUL_ERROR2("fail to verify_etc_5, sum_gcm_correct(%0) != instance_correct_count(%1).", sum_gcm_correct, instance_correct_count);

	// verify_etc_6 : check confusion matrix with all instances scan.
	auto cfmbai = get_db().get_confusion_matrix_by_all_instances();
	common::verify_confusion_matrix(gcm, cfmbai);
}

// only called from DEBUG or tests.
// some values use cache or intermediate values for performance,
// and make sure the values match the actual values.
inline void supul_t::model::verify_all(void) {
	if (not initialized) return;
	common::function_logger l{__func__, "model", common::function_logger::show_option::full_without_line};
	verify_cache();
	verify_global();
	verify_etc();
}

inline void supul_t::model::update_generation_etc(_in int64_t generation_id, _in const type::map_variant& before_global, _in const int64_t weak_instance_count, _in const int64_t before_weak_instance_correct_count, _in const int64_t after_weak_instance_correct_count) {
	auto after_global = get_db().get_global();

	if (not weak_instance_count) THROW_SUPUL_INTERNAL_ERROR0;

	// the instance_count must be the same before and after the rebuild.
	int64_t instance_count = common::get_variant_int64(before_global, "instance_count");
	if (instance_count != common::get_variant_int64(after_global, "instance_count")) THROW_SUPUL_ERROR("instance_count changed.");
	if (instance_count == 0) THROW_SUPUL_INTERNAL_ERROR0;

	// calc.
	double weak_instance_ratio = static_cast<double>(weak_instance_count) / static_cast<double>(instance_count);
	double before_weak_instance_accuracy = static_cast<double>(before_weak_instance_correct_count) / static_cast<double>(weak_instance_count);
	double after_weak_instance_accuracy  = static_cast<double>(after_weak_instance_correct_count)  / static_cast<double>(weak_instance_count);
	double before_instance_accuracy		 = common::get_variant_double(before_global, "instance_accuracy");
	double after_instance_accuray		 = common::get_variant_double(after_global,  "instance_accuracy");

	// update.
	get_db().update_generation_etc(generation_id, instance_count, weak_instance_count, weak_instance_ratio, before_weak_instance_accuracy, 
								   after_weak_instance_accuracy, before_instance_accuracy, after_instance_accuray);
}

inline void supul_t::model::chunk_limit(_in int64_t lower_bound, _in int64_t upper_bound) {
	std::vector<int64_t> chunk_ids_to_remove;

	if (lower_bound > upper_bound) THROW_SUPUL_ERROR2("invalid bound, %0, %1.", lower_bound, upper_bound);
	if (upper_bound <= 0) THROW_SUPUL_ERROR1("invalid upper bound, %1.", upper_bound);
	auto g = get_db().get_global();
	auto instance_count = common::get_variant_int64(g, "instance_count");
	if (instance_count <= upper_bound) {
		// under upper_bound. do nothing...
		return;
	}

	// do chunk remove.
	gaenari::logger::warn("chunk limit, instance_count={0}, upper_bound={1}.", {instance_count, upper_bound});

	// remove chunks till lower_bound.
	// get chunk information in oldest order.
	get_db().get_chunk_list([&](const auto& row) -> bool {
		// get chunk id, total_count.
		auto chunk_id = common::get_variant_int64(row, "id");
		auto chunk_total_count = common::get_variant_int64(row, "total_count");

		if (instance_count - chunk_total_count < lower_bound) {
			// remove stop.
			return false;
		}

		// do not remove here.
		// query is in progress.
		// to support databases that do not support delete-while-query.
		chunk_ids_to_remove.emplace_back(chunk_id);
		instance_count -= chunk_total_count;
		if (instance_count < 0) THROW_SUPUL_INTERNAL_ERROR0;

		return true;
	});

	gaenari::logger::info("chunk_id to be removed: {0}.", {gaenari::common::vec_to_string(chunk_ids_to_remove)});

	// delete chunks.
	for (const auto& chunk_id: chunk_ids_to_remove) {
		remove_chunk(chunk_id);
	}

	// get global after remove.
	auto g1 = get_db().get_global();
	auto instance_count1 = common::get_variant_int64(g1, "instance_count");
	if (not ((lower_bound <= instance_count1) && (instance_count1 < upper_bound))) {
		// not removed?
		THROW_SUPUL_INTERNAL_ERROR0;
	}

	if (instance_count != instance_count1) {
		THROW_SUPUL_ERROR2("instance_count not matched, calc=%0, global=%1.", instance_count, instance_count1);
	}
}

inline void supul_t::model::remove_chunk(_in size_t chunk_id) {
	int64_t correct = 0;
	int64_t total = 0;
	int64_t total_weak_count = 0;
	std::map<int64_t, std::map<int64_t, int64_t>> cm;	// confusion_matrix update.	(cm[actual][predicted]=count)
	std::unordered_map<int64_t, int64_t> leaf_info_id_count;
	std::unordered_map<int64_t, int64_t> leaf_info_id_correct_count;
	auto& db = get_db();

	// get the evaluation result of a target chunk. subtract the value later.
	db.get_leaf_info_by_chunk_id(chunk_id, [&](const auto& row) -> bool {
		// leaf_info_id_count update.
		auto id = common::get_variant_int64(row, "id");
		leaf_info_id_count[id]++;

		// for confusion matrix update.
		auto actual    = common::get_variant_int64(row, "instance.actual");
		auto predicted = common::get_variant_int64(row, "label_index");
		cm[actual][predicted]++;
		if (actual == predicted) { 
			correct++;
			leaf_info_id_correct_count[id]++;
		}
		
		total++;
		total_weak_count += common::get_variant_int64(row, "instance_info.weak_count");
		return true;
	});

	// the chunk is updated?
	auto updated = db.get_chunk_updated(chunk_id);

	// total count check.
	auto total_count = db.get_total_count_by_chunk_id(chunk_id);
	int64_t updated_count = 0;
	if (updated) {
		if (total_count != total) THROW_SUPUL_ERROR2("total count mis-matching, %0 != %1.", total_count, total);
		updated_count = total_count;
	}

	// update global confusion matrix.
	for (const auto& it1: cm) {
		const auto& actual = it1.first;
		for (const auto& it2: it1.second) {
			const auto& predicted = it2.first;
			const auto& increment = it2.second;
			db.update_global_confusion_matrix_item_increment(actual, predicted, -increment);
		}
	}

	// update leaf_info.
	for (const auto& it: leaf_info_id_count) {
		auto& id = it.first;
		auto& count = it.second;
		db.update_leaf_info(id, -leaf_info_id_correct_count[id], -leaf_info_id_count[id]);
	}

	// get global.
	auto g = db.get_global();
	int64_t g_instance_count = common::get_variant_int64(g, "instance_count") - total_count;
	int64_t g_updated_instance_count  = common::get_variant_int64(g, "updated_instance_count") - updated_count;
	int64_t g_instance_correct_count  = common::get_variant_int64(g, "instance_correct_count") - correct;
	int64_t g_acc_weak_instance_count = common::get_variant_int64(g, "acc_weak_instance_count") - total_weak_count;
	double g_instance_accuracy = 0.0;
	if (g_updated_instance_count != 0) {
		g_instance_accuracy = static_cast<double>(g_instance_correct_count) / static_cast<double>(g_updated_instance_count);
	}

	// error?
	if ((g_instance_count < 0) or (g_updated_instance_count < 0) or (g_instance_correct_count < 0) or (g_acc_weak_instance_count < 0))
		THROW_SUPUL_INTERNAL_ERROR0;

	// update global.
	db.set_global({
		{"instance_count",			g_instance_count},
		{"updated_instance_count",	g_updated_instance_count},
		{"instance_correct_count",	g_instance_correct_count},
		{"acc_weak_instance_count",	g_acc_weak_instance_count},
		{"instance_accuracy",		g_instance_accuracy}
	});

	// delete records(call order is important).
	db.delete_instance_by_chunk_id(chunk_id);
	db.delete_instance_info_by_chunk_id(chunk_id);
	db.delete_chunk_by_id(chunk_id);

	// some leaf_info changed, so clear all cache.
	clear_all_cache();
}

} // supul
} // supul

#endif // HEADER_GAENARI_SUPUL_SUPUL_IMPL_MODEL_HPP
