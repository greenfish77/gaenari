#ifndef HEADER_GAENARI_GAENARI_METHOD_STRINGFY_TREE_HPP
#define HEADER_GAENARI_GAENARI_METHOD_STRINGFY_TREE_HPP

namespace gaenari {
namespace method {
namespace stringfy {

static inline const std::string& value_to_string(_in const gaenari::type::value& value, _in const common::string_table& strings, _out std::string& _) {
	if (value.valueype == gaenari::type::value_type::value_type_double) { _ = common::dbl_to_string(value.numeric_double); return _; }
	if (value.valueype == gaenari::type::value_type::value_type_size_t) return strings.get_string(value.index);
	if (value.valueype == gaenari::type::value_type::value_type_int)    { _ = std::to_string(value.numeric_int32); return _; }
	if (value.valueype == gaenari::type::value_type::value_type_int64)  { _ = std::to_string(value.numeric_int64); return _; }
	THROW_GAENARI_ERROR("invalid value type.");
	_.clear();
	return _;
}

// support mime_type:
//    - text/plain
//    - text/colortag
inline std::string tree(_in const decision_tree::tree_node& root, _in const std::vector<dataset::column_info>& columns, _in const common::string_table& strings, _in const std::string& mime_type, _in bool show_treenode_id, _option_out size_t* node_count=nullptr, _option_in const stringfy::style& style = stringfy::style()) {
	size_t _node_count = 0;
	std::string ret;
	std::string treenode_id;
	std::string _;

	// color setting.
	struct {
		std::string name1,  name2;
		std::string value1, value2;
		std::string label1, label2;
		std::string score1, score2;
		std::string id1,    id2;
	} clr;

	// get mime type.
	auto type = stringfy::util::get_mime_type(mime_type);
	if (not ((type == stringfy::type::text_plain) or (type == stringfy::type::text_colortag))) {
		THROW_GAENARI_ERROR("invalid mime type.");
	}

	// set color theme.
	if (type == stringfy::type::text_colortag) {
		clr.name1  = '<' + style.name  + '>'; clr.name2  = "</" + style.name  + '>';
		clr.value1 = '<' + style.value + '>'; clr.value2 = "</" + style.value + '>';
		clr.label1 = '<' + style.label + '>'; clr.label2 = "</" + style.label + '>';
		clr.score1 = '<' + style.score + '>'; clr.score2 = "</" + style.score + '>';
		clr.id1    = '<' + style.id    + '>'; clr.id2    = "</" + style.id    + '>';
	} else if (type == stringfy::type::text_plain) {
		// no color information('').
		// do nothing.
	}

	// immutable traverse tree.
	decision_tree::util::traverse_tree_node_const(root, [&](auto& treenode, int depth) -> bool {
		// calc node count.
		_node_count++;

		// skip root (the root itself is omitted.)
		if (depth <= 0) return true;

		// treenode id.
		if (show_treenode_id) {
			ret += clr.id1 + common::fixed_length(std::to_string(treenode.id), 8, ' ') + clr.id2 + ' ';
		}

		// indent spaces.
		ret += std::string((static_cast<size_t>(depth)-1) * 4, ' ');

		// feature name
		ret += clr.name1 + columns[treenode.rule.feature_indexes[0]].name + clr.name2;

		// operator
		ret += ' ' + stringfy::util::get_op(treenode.rule.type) + ' ';

		// value
		ret += clr.value1 + value_to_string(treenode.rule.args[0], strings, _) + clr.value2;

		// leaf node
		if (treenode.leaf) {
			const auto& leaf_info = treenode.leaf_info;
			// delimiter
			ret += ": ";

			// label name
			ret += clr.label1 + strings.get_string(leaf_info.label_string_index) + clr.label2;

			// score
			ret += clr.score1;
			ret += " (";
			ret += std::to_string(leaf_info.correct_count + leaf_info.incorrect_count);	// total count
			ret += '/';
			ret += std::to_string(leaf_info.incorrect_count);							// incorrect count
			ret += ')';
			ret += clr.score2;
		}

		// next line
		ret += '\n';

		// goto next depth.
		return true;
	});

	if (node_count) *node_count = _node_count;

	return ret;
}

} // stringfy
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_STRINGFY_TREE_HPP
