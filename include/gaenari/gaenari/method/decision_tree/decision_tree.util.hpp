#ifndef HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_UTIL_HPP
#define HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_UTIL_HPP

namespace gaenari {
namespace method {
namespace decision_tree {
namespace util {

// there may be several types of rule operation (currently there is only this one).
// type 1 has two operands of the form `A(feature value) OPERATOR B(rule value)`.
inline bool is_match_type_1(_in const type::value& feature_value, _in const rule_t& rule) {
	switch (rule.type) {
		case rule_t::rule_type::cmp_equ:
			if (feature_value == rule.args[0]) return true;
			break;
		case rule_t::rule_type::cmp_lte:
			if (feature_value <= rule.args[0]) return true;
			break;
		case rule_t::rule_type::cmp_lt:
			if (feature_value <  rule.args[0]) return true;
			break;
		case rule_t::rule_type::cmp_gt:
			if (feature_value >  rule.args[0]) return true;
			break;
		case rule_t::rule_type::cmp_gte:
			if (feature_value >= rule.args[0]) return true;
			break;
		default:
			THROW_GAENARI_ERROR("not supported rule type.");
	}
	return false;
}

// match rule with one value string.
// use other kind of data like json rather than dataframe.
// strings_map can be dataframe::_strings_map.
inline bool is_match_type_1(_in const std::string& feature_value, _in const rule_t& rule, _in const common::string_table& strings) {
	// to index
	if (type::value_type::value_type_size_t == rule.args[0].valueype) {
		auto find = strings.get_id_size_t(feature_value);
		if (find == std::numeric_limits<size_t>::max()) THROW_GAENARI_FEATURE_NOT_FOUND("feature not found:" + feature_value);
		return is_match_type_1(find, rule);
	}
	// to double
	if (type::value_type::value_type_double == rule.args[0].valueype) return is_match_type_1(std::stod(feature_value), rule);
	if (type::value_type::value_type_int    == rule.args[0].valueype) return is_match_type_1(std::stoi(feature_value), rule);
	if (type::value_type::value_type_int64  == rule.args[0].valueype) return is_match_type_1(static_cast<int64_t>(std::stoll(feature_value)), rule);
	THROW_GAENARI_ERROR("invalid data type.");
	return false;
}

// match rule with one value string.
// use other kind of data like json rather than dataframe.
// strings_map can be dataframe::_strings_map.
inline bool is_match_type_1(_in const std::variant<std::monostate,int,size_t,std::string,double,int64_t>& feature_value, _in const rule_t& rule, _in const common::string_table& strings) {
	auto index = feature_value.index();
	if (index == 1) return is_match_type_1(std::get<1>(feature_value), rule);
	if (index == 2) return is_match_type_1(std::get<2>(feature_value), rule);
	if (index == 4) return is_match_type_1(std::get<4>(feature_value), rule);
	if (index == 5) return is_match_type_1(std::get<5>(feature_value), rule);
	THROW_GAENARI_ERROR("invalid data type.");
}

// match rule with (feature name,value) map.
// use other kind of data like json rather than dataframe.
// columns can be dataframe::_column.
// strings_map can be dataframe::_strings_map.
inline bool is_match_type_1(_in const std::map<std::string,std::string>& key_value, _in const rule_t& rule, _in const common::string_table& strings, _in const std::vector<dataset::column_info>& columns) {
	const std::string& feature_name = columns[rule.feature_indexes[0]].name;
	const auto find = key_value.find(feature_name);
	if (find == key_value.end()) THROW_GAENARI_FEATURE_NOT_FOUND("feature not found:" + feature_name);
	return is_match_type_1(find->second, rule, strings);
}

inline bool is_match_type_1(_in const std::map<std::string,std::variant<std::monostate,int,size_t,std::string,double,int64_t>>& key_value, _in const rule_t& rule, _in const common::string_table& strings, _in const std::vector<dataset::column_info>& columns) {
	const std::string& feature_name = columns[rule.feature_indexes[0]].name;
	const auto find = key_value.find(feature_name);
	if (find == key_value.end()) THROW_GAENARI_FEATURE_NOT_FOUND("feature not found:" + feature_name);
	return is_match_type_1(find->second, rule, strings);
}

// match rule with data.
inline bool is_match(_in const dataset::dataframe& df, _in const rule_t& rule, _in size_t row_index) {
	// alias
	const auto& columns       = df.columns();
	const auto  feature_value = df.get_value(row_index, rule.feature_indexes[0]);
	// get type 1 result
	return is_match_type_1(feature_value, rule);
}

inline std::vector<size_t> get_matched_row_selections(_in const dataset::dataset& train, _in rule_t& rule, _in const std::vector<size_t>& old_row_selections) {
	std::vector<size_t> r;
	for (auto row_index: old_row_selections) {
		if (is_match(train.x, rule, row_index)) r.push_back(row_index);
	}
	return r;
}

// traverse

// ex)
// _traverse_tree_node[_const](node, [](auto& treenode, int depth) -> bool {
//		treenode;		// current traversing tree node.
//		depth;			// treenode depth. (0: root)
//		return true;	// return true(continue), false(stop)
//	}

// internal traverse function.
template <typename T=tree_node>
using fn_traverse_t = std::function<bool(_in T& current, _in int depth)>;
template <typename T=tree_node>
inline void _traverse_tree_node_internal(_in T& root, _in const fn_traverse_t<T>& fn_traverse) {
	std::stack<stack_traverse_node<T>> stack;

	// start with root
	stack.emplace(stack_traverse_node<T>{root,0});

	for (;;) {
		if (stack.empty()) break;
		auto current = stack.top();
		stack.pop();

		// before push on the stack, call callback function.
		// child can be changed. (by calling _traverse_tree_node)
		if (not fn_traverse(current.node, current.depth)) break;

		// child push to stack
		for (auto& child: current.node.childs) stack.emplace(stack_traverse_node<T>{*child,current.depth+1});
	}
}

// traverse(depth first search) the tree immutably.
// it's more safe.
// ex)
// _traverse_tree_node_const(node, [](auto& treenode, int depth) -> bool {
//     ...
//     (auto& = const tree_node&)
inline void traverse_tree_node_const(_in const tree_node& root, _in const fn_traverse_t<const tree_node>& fn_traverse) {
	_traverse_tree_node_internal<const tree_node>(root, fn_traverse);
	return;
}

// traverse(depth first search) the tree mutably.
// you can change the childs, and you need to change the parent.
// use with caution.
// ex)
// _traverse_tree_node(node, [](auto& treenode, int depth) -> bool {
//     ...
//     (auto& = tree_node&)
inline void traverse_tree_node(_in tree_node& root, _in const fn_traverse_t<tree_node>& fn_traverse) {
	_traverse_tree_node_internal<tree_node>(root, fn_traverse);
	return;
}

// terminal nodes means that has only leaf nodes.
// terminal_node = V
//     f1 = V1: C1 (...)
//     f1 = V2: C2 (...)
inline bool is_terminal_node(_in const tree_node& node) {
	if (node.leaf) return false;
	for (const auto& child: node.childs) {
		if (not child->leaf) return false;
	}
	return true;
}

// check if the label count of each split is less than the minimum value.
inline bool is_have_min_instance(_in const std::vector<split_info>& split_infos, _in size_t min_instances) {
	for (const auto& split_info: split_infos) {
		if (common::map_count(split_info.predicted_count.count) < min_instances) return true;
	}
	return false;
}

// get a value list of class label of the leaf nodes of the tree.
inline void get_label_values(_in const tree_node* _root, _out std::vector<size_t>& label_values) {
	std::set<size_t> _label_values_unique;

	if (not _root) THROW_GAENARI_ERROR("tree is not built.");

	// clear output
	label_values.clear();

	// traverse tree nodes.
	util::traverse_tree_node_const(*_root, [&label_values, &_label_values_unique](const tree_node& treenode, int depth) -> bool {
		// only treenode.
		if (not treenode.leaf) return true;

		// label push back.
		if (_label_values_unique.end() == _label_values_unique.find(treenode.leaf_info.label_string_index)) {
			label_values.push_back(treenode.leaf_info.label_string_index);
			_label_values_unique.insert(treenode.leaf_info.label_string_index);
		}

		return true;
	});
}

// called from decision_tree::eval(...)
inline void get_actual_label_index(_in const std::vector<std::map<std::string,std::string>>& tests, _in const std::string& y_name, _in const common::string_table& strings, _out std::vector<size_t>& actuals) {
	const std::string empty_string;

	// clear output
	actuals.clear();

	// get label value as string and index of each items.
	for (auto& test: tests) {
		const auto& label_value  = common::map_find(test, y_name, empty_string);
		size_t      label_index  = strings.get_id_size_t(label_value);
		if (label_value == empty_string) label_index = std::numeric_limits<size_t>::max();
		actuals.push_back(label_index);
	}
}

// called from decsion_tree_t::eval(...)
inline void get_actual_label_index(_in const dataset::dataframe& tests, _in const std::string& y_name, _in const common::string_table& strings, _out std::vector<size_t>& actuals) {
	// clear output
	actuals.clear();

	// find the label column index.
	bool found = false;
	size_t label_index = 0;
	for (auto& column: tests.columns()) {
		if (column.name == y_name) {
			found = true;
			break;
		}
		label_index++;
	}
	if (not found) THROW_GAENARI_ERROR("not found label column.");

	// get label value as string and index of each rows
	for (auto& test: tests.iter_string()) {
		const auto& label_value = test.values[label_index];
		size_t      label_index = strings.get_id_size_t(label_value);
		actuals.push_back(label_index);
	}
}

// 0 -> 'a', 1 -> 'b', ..., 26 -> 'aa', ..., 702 -> 'aaa', 703 -> 'aab', ...
template <typename T>
inline std::string get_shorten_word(_in T i) {
	std::string ret;
	for (;;) {
		T remainder = i % 26;
		ret.insert(ret.begin(), 'a' + static_cast<char>(remainder));
		if (i < 26) break;
		i = (i / 26) - 1;
	}
	return ret;
}

//   a  b   <-- classified as
//  .. .. |   a = <C1>
//  .. .. |   b = <C2>
inline std::string get_confusion_matrix_plain_text(_in const std::map<size_t,std::map<size_t,size_t>>& confusion, _in const common::string_table& strings, _in const std::vector<size_t>& label_values) {
	const int margin = 1;
	constexpr auto unknown = std::numeric_limits<size_t>::max();
	std::string ret;
	std::string word;
	std::vector<std::vector<size_t>> count;
	auto _confusion = confusion;			// deep copied from confusion.
	auto _col_label_values = label_values;	// deep copied from label_values.
	auto _row_label_values = label_values;	// deep copied from label_values.

	if (confusion.empty()) THROW_GAENARI_ERROR("empty confusion matrix.");

	// the number of columns in all rows must be the same.
	// as an exception, one `unknown` may be added.
	size_t column_count = _confusion.begin()->second.size();
	if (column_count == _col_label_values.size() + 1) _col_label_values.push_back(unknown);	// add unknown.
	if (column_count != _col_label_values.size()) THROW_GAENARI_INTERNAL_ERROR0;
	for (const auto& row: _confusion) if (row.second.size() != column_count) THROW_GAENARI_INTERNAL_ERROR0;

	// as an exception to the row, one `unknown` can be added.
	size_t row_count = _confusion.size();
	if (row_count == _row_label_values.size() + 1) _row_label_values.push_back(unknown);	// add unknown.

	// 2d vector for count.
	count.resize(_confusion.size());
	for (size_t i=0; i<_confusion.size(); i++) count[i].resize(column_count);

	// _confusion matrix(map) -> count matrix(vector)
	for (size_t row=0; row<_confusion.size(); row++) {
		size_t row_label_index = _row_label_values[row];
		for (size_t col=0; col<column_count; col++) {
			size_t col_label_index = _col_label_values[col];
			count[row][col] = _confusion[row_label_index][col_label_index];
		}
	}

	// get each column width(no margin space).
	std::vector<size_t> width;
	width.resize(column_count);
	// initially, the length of the header name(a,b,c,...)
	for (size_t i=0; i<column_count; i++) width[i] = get_shorten_word(i).length();
	for (size_t row=0; row<_confusion.size(); row++) {
		for (size_t col=0; col<column_count; col++) {
			auto count_value = std::to_string(count[row][col]);
			if (width[col] < (int)count_value.size()) width[col] = count_value.size();
		}
	}

	// let's do it.
	size_t max_alias_length = 0;
	for (size_t row=0; row<_confusion.size(); row++) {
		// add header.
		if (row == 0) {
			for (size_t col=0; col<column_count; col++) {
				auto alias = get_shorten_word(col);
				auto word  = common::fixed_length(alias, width[col] + margin, ' ', 0, margin, false, true);
				ret += word;
				if (max_alias_length < alias.length()) max_alias_length = alias.length();
			}
			ret += "   <-- classified as\n";
		}

		// add row.
		for (size_t col=0; col<column_count; col++) {
			auto word = common::fixed_length(std::to_string(count[row][col]), width[col] + margin, ' ', 0, margin, false, true);
			ret += word;
		}
		ret += " |   ";
		ret += common::fixed_length(get_shorten_word(row), max_alias_length + margin, ' ', margin, 0, false, true);
		ret += "= ";
		ret += _row_label_values[row] == unknown ? "(unknown)" : strings.get_string_noexept(_row_label_values[row]);
		ret += "\n";
	}

	// a b c   <-- classified as			a b c   <-- classified as
	// 0 0 1 |   a = G1				==>		0 0 1 |   a = G1
	// 1 0 0 |   b = G0						1 0 0 |   b = G0
	//											  |   c = (unknown)
	if (_col_label_values.size() == _row_label_values.size() + 1) {
		for (size_t col=0; col<column_count; col++) ret += std::string(width[col] + margin, ' ');
		ret += " |   ";
		ret += common::fixed_length(get_shorten_word(_confusion.size()), max_alias_length + margin, ' ', margin, 0, false, true);
		ret += "= (unknown)\n";
	}

	return ret;
}

} // util
} // decision_tree
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_UTIL_HPP
