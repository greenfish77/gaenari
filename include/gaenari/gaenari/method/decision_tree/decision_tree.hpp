#ifndef HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_H
#define HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_H

namespace gaenari {
namespace method {
namespace decision_tree {

class decision_tree {
public:
	decision_tree()  = default;
	~decision_tree() {
		clear();
	}

public:
	// train a decision tree.
	// - train             : dataset for training
	// - split_strategy    : [split_strategy_default] only default split strategy is supported.
	// - min_instances     : [2.0] the minimum number of instances per split.
	// - pruning_weight    : [1.2] pruning weight. more weight, more pruning. weight must be over than 1.0(=do not pruning).
	// - early_stop_weight : [0.0] pre-pruning weight for early stop. more weight, more pruning. weight must be over than 0.0(=do not pruning).
	void train(_in const dataset::dataset& train, _in split_strategy split_strategy=split_strategy::split_strategy_default, _in size_t min_instances=2.0, _in double pruning_weight=1.2, _in double early_stop_weight=0.0);

	// clear tree.
	void clear(void);

	// check empty.
	bool empty(void);

	// get tree.
	const tree_node& get_tree(void) const;

	// predict with one row (feature name, value) map data.
	// returns predicted label index and the leaf tree node id.
	//
	// it's the main function of the predict function family.
	// value type is std::string. it's automatically converted to the data type defind in the tree.
	// it's used for external data other than dataframe, such as json.
	// if the given test data does not match the tree, max size_t is returend.
	// 
	// it's used for external data other than dataframe, such as json.
	// if the given test data does not match the tree, max size_t is returend, and id = -1.
	size_t predict(_in const std::map<std::string,std::string>& test, _option_out int* id = nullptr) const;

	// predict with one row (feature name, value) map data.
	// returns predicted label index and the leaf tree node id.
	//
	// value type is variant from dataframe row.
	size_t predict(_in const std::map<std::string,std::variant<std::monostate,int,size_t,std::string,double,int64_t>>& test, _option_out int* id = nullptr) const;

	// predict with multiple rows (feature name, value) map data.
	// other than that, it is the same as the above function(size_t predict(_in const std::map<std::string,std::string>& test)).
	std::vector<size_t> predict(_in const std::vector<std::map<std::string,std::string>>& tests) const;

	// predict with dataframe.
	// decision_tree has vector strings for nominal strings. this vector strings depend on the training data.
	// as intended, the test data should have the same vector strings,
	// but for a flexible structure, a dataframe containing a different vector strings from the training time is also available.
	std::vector<size_t> predict(_in const dataset::dataframe& tests) const;

	// get the evaluation result of testset.
	//
	// T : `std::vector<std::map<std::string,std::string>>` or `dataset::dataframe`
	//      ----------------------------------------------      --------------------
	//      multiple (feature name,value) map                   dataframe
	//
	// confusion matrix format:
	//      std::map<size_t,std::map<size_t,size_t>> confusion
	//                 |               |      '---> count
	//                 |               '----------> predicted label index of this->_strings
	//                 '--------------------------> actual    label index of this->_strings
	//      confusion[actual][predicted] = count
	template <typename T=std::vector<std::map<std::string,std::string>>>
	void eval(_in const T& tests, _out double& accuracy, _option_out std::map<size_t,std::map<size_t,size_t>>* confusion=nullptr, _option_out std::vector<size_t>* predicteds=nullptr, _option_out size_t* correct_count=nullptr) const;

	// converts the confusion matrix to a string.
	// output format (similar to weka)
	//
	//   a  b   <-- classified as
	//  .. .. |   a = <C1>         --> horizental sum : actual count of <C1>
	//  ** .. |   b = <C2>
	//                           ._
	//   |                         `-> diagonal sum : correctly predicted count
	//   |
	//   '---------------------------> vertical sum : predicted count of <C1>
	//
	//  ex) ** count is the actual count of <C2>, but the count predicted as <C1>.
	std::string to_string_confusion_matrix(_in const std::map<size_t,std::map<size_t,size_t>>& confusion, _in const std::string& type="text/plain") const;

	// stringfy tree.
	std::string stringfy(_in const std::string& mime_type="text/plain", _in bool show_treenode_id=false) const;

protected:
	// decision tree root node
	tree_node* root = nullptr;

	// meta data from dataset.
	// these are related to trained this->root and training dataset.
	// without these, some informations in this->root are dangling.
	// that is, all tree nodes under this->root refer to the information below.
	//
	// to serialize decision_tree to a file or string, the following data need to be stored, too.
	// the entire full dataframe data is not required to save.
	std::vector<dataset::column_info> columns;
	common::string_table strings;
	std::string y_name;

	// these are not serializable.
	// _label_values is come from _root. (decision_tree::util::_get_labels(...))
	std::vector<size_t> label_values;
};

// implementation.

inline void decision_tree::train(_in const dataset::dataset& train, _in split_strategy split_strategy, _in size_t min_instances, _in double pruning_weight, _in double early_stop_weight) {
	size_t index = 0;
	int treenode_id = 0;
	bool error = false;
	std::vector<size_t> row_selections;	// row sampling
	std::stack<stack_train_node> stack;	// internal stack (do not use recursive function call)
	leaf_info_t _root_leaf_info;

	if (root) THROW_GAENARI_ERROR("already trained.");
	if ((min_instances == 0) or (pruning_weight < 1.0) or (early_stop_weight < 0)) THROW_GAENARI_INVALID_PARAMETER("invalid train parameter.");
	if (train.y.columns().size() != 1) THROW_GAENARI_ERROR("invalid dataset.");

	// first, full row selection (0, 1, 2, ..., instance count - 1)
	row_selections.resize(train.metadata.instance_count);
	std::iota(row_selections.begin(), row_selections.end(), 0);

	// build root node and get classify information and label count
	root = tree_node::build_root_node(treenode_id);
	root->predicted_count.count = 
		engine::get_leaf_info(train, row_selections, _root_leaf_info);

	// push the root node to stack with row selections matched.
	stack.emplace(stack_train_node(*root, row_selections));

	for (;;) {
		if (stack.empty()) break;
		auto current = std::move(stack.top());	// get stack top and pop. (std::move is not affected yet. just hard-copy.)
		stack.pop();

		// get split
		auto split_infos = engine::split_tree(train, current.row_selections, split_strategy, min_instances);
		std::reverse(split_infos.begin(), split_infos.end());

		// check early stop for pruning?
		if (not split_infos.empty()) {
			// check only non-leaf.

			// do early stop (pre-pruning).
			//  . calc error rate of current node : err_current
			//  . calc error rate of childs       : err_childs
			// split error rate * weight > parent error rate
			// if err_childs * weight > err_current, do not split and turn to leaf node.
			// if weight == 0.0, do not early stop.
			if (engine::check_early_stop_for_pruning(current.node, split_infos, early_stop_weight)) {
				// do early stop
				split_infos.clear();
			}
		}

		if (split_infos.empty()) {
			// stop criteria, it's leaf node.
			current.node.leaf = true;

			// get classify information and label count
			current.node.predicted_count.count = 
				engine::get_leaf_info(train, current.row_selections, current.node.leaf_info);

			// skip child processing
			continue;
		}

		// do split
		auto childs = tree_node::build_child_nodes(current.node, split_infos.size(), treenode_id);
		if (childs.empty()) THROW_GAENARI_ERROR("empty build_child_nodes.");

		// child loop
		index = 0;
		for (auto* child: childs) {
			// copy split info to tree node.
			child->rule            = std::move(split_infos[index].rule); // (std::move is not affected yet. just hard-copy.)
			child->predicted_count = std::move(split_infos[index].predicted_count);

			// get matched new row indexes from current old row indexes
			auto new_matched_indexes = util::get_matched_row_selections(train, child->rule, current.row_selections);

			// stack push the child to stack with row selections matched.
			stack.emplace(stack_train_node(*child, new_matched_indexes));

			index++;
		}
	}

	// get meta data
	// remark)
	//   - string table size is big, so decision tree's string table is reference of dataframe.
	//   - if the dataframe is destroyed, its string table becomes a dangling pointer.
	//   - if you want to prevent it, use 
	//         _strings = _strings;
	//     or
	//         _strings.reference_from_const(&strings);
	//         ...
	//         _strings.copy_from_reference();
	//         ...
	//         df.clear();
	columns = train.x.columns();
	strings.reference_from_const(&train.x.get_shared_data_const().strings);
	y_name = train.y.columns()[0].name;
	util::get_label_values(root, label_values);

	// post-processing
	for (;;) {
		// post-processing continues until there is no change.
		auto pruned   = engine::post_pruning(*root, pruning_weight);
		auto combined = engine::combine_same_label(*root);
		if (not pruned and not combined) break;
	}

	return;
}

inline void decision_tree::clear(void) {
	if (not root) return;
	root->destroy_all_child_node(true);
	root = nullptr;
}

inline bool decision_tree::empty(void) {
	if (root and (not root->childs.empty())) return false;
	return true;
}

inline const tree_node& decision_tree::get_tree(void) const {
	if (not root) THROW_GAENARI_INTERNAL_ERROR0;
	return *root;
}

inline size_t decision_tree::predict(_in const std::map<std::string,std::variant<std::monostate,int,size_t,std::string,double,int64_t>>& test, _option_out int* id/*=nullptr*/) const {
	bool matched = false;

	if (id) *id = -1;
	if (not root) THROW_GAENARI_ERROR("tree is not built.");

	const auto* current = root;
	for (;;) {
		// loop to leaf node.
		if (current->leaf) {
			if (id) *id = current->id;
			break;
		}
		matched = false;
		// which child is matched?
		for (const auto child: current->childs) {
			if (util::is_match_type_1(test, child->rule, strings, columns)) {
				// found, goto child.
				matched = true;
				current = child;
				break;
			}
		}
		// does not match childs of current node.
		// it may be a case where a string value that did not exist when learning the current node was input.
		if (not matched) return std::numeric_limits<size_t>::max();
	}

	// return string index of class.
	return current->leaf_info.label_string_index;
}

inline size_t decision_tree::predict(_in const std::map<std::string,std::string>& test, _option_out int* id/*=nullptr*/) const {
	bool matched = false;

	if (id) *id = -1;
	if (not root) THROW_GAENARI_ERROR("tree is not built.");

	const auto* current = root;
	for (;;) {
		// loop to leaf node.
		if (current->leaf) {
			if (id) *id = current->id;
			break;
		}
		matched = false;
		// which child is matched?
		for (const auto child: current->childs) {
			if (util::is_match_type_1(test, child->rule, strings, columns)) {
				// found, goto child.
				matched = true;
				current = child;
				break;
			}
		}
		// does not match childs of current node.
		// it may be a case where a string value that did not exist when learning the current node was input.
		if (not matched) return std::numeric_limits<size_t>::max();
	}

	// return string index of class.
	return current->leaf_info.label_string_index;
}

inline std::vector<size_t> decision_tree::predict(_in const std::vector<std::map<std::string,std::string>>& tests) const {
	std::vector<size_t> ret;

	// pre-allocating memory with reserve.
	ret.reserve(tests.size());

	// get prediction.
	for (const auto& test: tests) {
		size_t label = 0;
		try {
			label = predict(test);
		} catch (exceptions::error_feature_not_found& /*e*/) {
			// if it fails due to insufficeient features, it is treated as `unknown` prediction.
			label = std::numeric_limits<size_t>::max();
		}
		ret.push_back(label);
	}

	return ret;
}

inline std::vector<size_t> decision_tree::predict(_in const dataset::dataframe& tests) const {
	std::map<std::string,std::string> map_data;
	std::string empty_string = "";
	const auto& columns = tests.columns();
	std::vector<size_t> ret;

	// every row in dataframe.
	for (const auto& row: tests.iter_string()) {
		// every column in row.
		size_t index = 0;
		for (const auto& value: row.values) {
			map_data[columns[index].name] = value;
			index++;
		}
		// predict
		auto label = predict(map_data);
		ret.push_back(label);
	}

	return ret;
}

template <typename T>
inline void decision_tree::eval(_in const T& tests, _out double& accuracy, _option_out std::map<size_t,std::map<size_t,size_t>>* confusion, _option_out std::vector<size_t>* predicteds, _option_out size_t* correct_count) const {
	constexpr size_t unknown = std::numeric_limits<size_t>::max();
	std::vector<size_t> _predicteds;
	std::vector<size_t> _actuals;
	if (not predicteds) predicteds = &_predicteds;

	// add size_t max to label value. this is a space for an unknown label.
	auto _label_values = label_values;
	_label_values.push_back(unknown);

	// clear output
	if (confusion) confusion->clear();
	predicteds->clear();
	accuracy = 0.0;

	if (not root) THROW_GAENARI_ERROR("tree is not built.");

	// because it may not be displayed if the number of items,
	// set label value items in the confusion matrix in advance.
	if (confusion) {
		for (const auto& actual: _label_values) {
			for (const auto& predicted: _label_values) {
				(*confusion)[actual][predicted] = 0;
			}
		}
	}

	// get predicted.
	*predicteds = predict(tests);

	// pre-allocating memory with reserve.
	_actuals.reserve(tests.size());

	// get actual label values.
	util::get_actual_label_index(tests, y_name, strings, _actuals);

	// compare actual and predict.
	size_t count                     = _actuals.size();
	size_t count_without_unknown     = 0;
	size_t incorrect_without_unknown = 0;
	// the number of actual and predict must be the same.
	if (_actuals.size() != predicteds->size()) THROW_GAENARI_INTERNAL_ERROR0;
	for (size_t i=0; i<count; i++) {
		// except unknown
		if (not ((_actuals[i] == unknown) or ((*predicteds)[i] == unknown))) {
			count_without_unknown++;
			if (_actuals[i] != (*predicteds)[i]) incorrect_without_unknown++;
		}
		if (confusion) (*confusion)[_actuals[i]][(*predicteds)[i]]++;
	}

	// accuracy excludes unknown.
	if (count_without_unknown != 0) {
		accuracy = 1.0 - (static_cast<double>(incorrect_without_unknown) / static_cast<double>(count_without_unknown));
	}

	// set correct_count.
	if (correct_count) {
		if (incorrect_without_unknown > count_without_unknown) THROW_GAENARI_INTERNAL_ERROR0;
		*correct_count = count_without_unknown - incorrect_without_unknown;
	}

	// remove unneccessay `unknown`
	// 1) if all unknown rows are 0, remove.
	// 2) if all unknown cols are 0, remove.
	if (confusion) {
		auto found = true;
		for (const auto col: (*confusion)[unknown]) if (col.second != 0) found = false;
		if (found) (*confusion).erase(unknown);
		found = true;
		for (auto& row: (*confusion)) if (row.second[unknown] != 0) found = false;
		if (found) for (auto& row: (*confusion)) row.second.erase(unknown);
	}
}

inline std::string decision_tree::to_string_confusion_matrix(_in const std::map<size_t,std::map<size_t,size_t>>& confusion, _in const std::string& type) const {
	std::string ret;

	if (type != "text/plain") THROW_GAENARI_NOT_SUPPORTED_YET("not support yet.");
	if (not root) THROW_GAENARI_ERROR("tree is not built.");

	ret = util::get_confusion_matrix_plain_text(confusion, strings, label_values);

	return ret;
}

inline std::string decision_tree::stringfy(_in const std::string& mime_type, _in bool show_treenode_id) const {
	return method::stringfy::tree(*root, columns, strings, mime_type, show_treenode_id);
}

} // decision_tree
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_H
