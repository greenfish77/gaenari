#ifndef HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_ENGINE_HPP
#define HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_ENGINE_HPP

namespace gaenari {
namespace method {
namespace decision_tree {
class engine {
public:
	// there is a dependency on the function definition order, so struct is used instead of namespace.

	// split tree with current row selections.
	// return multple split infos.
	// empty return, stop criteria.
	static inline std::vector<split_info> split_tree(_in const dataset::dataset& train, _in const std::vector<size_t>& row_selections, _in split_strategy split_strategy, _in size_t min_instances) {
		auto found = false;
		size_t best_feature_index = 0;
		igr_result max_igr;

		// some values
		const auto feature_count = train.x.columns().size();
		max_igr.stop_criteria = true;
		max_igr.igr_value = std::numeric_limits<double>::lowest();

		// calc information gain ratio of each features.
		// and choose best one.
		auto S = calc_S<size_t>(train, row_selections);
		for (size_t feature_index=0; feature_index<feature_count; feature_index++) {
			// get information gain ratio and split info of current feature index.
			auto igr = calc_igr_main(train, row_selections, feature_index, S, split_strategy);

			// stop criteria. do not split.
			if (igr.stop_criteria) continue;

			// if there is a smaller instance count than min_instances in some split of the current feature index, it does not continue.
			// that is, it is checked while learning the tree, not the post pruning.
			if (util::is_have_min_instance(igr.split_infos, min_instances)) continue;

			// find max information gain ratio.
			if (igr.igr_value > max_igr.igr_value) {
				// best igr found
				found = true;
				max_igr = igr;
				best_feature_index = feature_index;
			}
		}

		if (not found) {
			// no best split? stop it.
			return {};
		}

		if (max_igr.stop_criteria) {
			// all features has no information gain ratio, so stop it.
			return {};
		}

		if (max_igr.igr_value == 0.0) {
			// all has same label.
			// stop it.
			return {};
		}

		// check split with all or nothing.
		// stop it.
		for (const auto& split_info: max_igr.split_infos) {
			size_t total_count = 0;
			for (const auto& count: split_info.predicted_count.count) total_count += count.second;
			if (total_count == row_selections.size()) return {};
		}
		
		// return maximum information gain ratio's split infos.
		return max_igr.split_infos;
	}

	// return label count as map(label index, count) of row selections.
	// max_count_index : majority label index
	// correct_count   : count of (max_count_index == index)
	// incorrect_count : count of (max_count_index != index)
	// static inline std::map<size_t,size_t> _get_leaf_info(_in const dataset::dataset& train, _in const std::vector<size_t>& row_selections, _out size_t& max_count_index, _out size_t& correct_count, _out size_t& incorrect_count) {
	static inline std::map<size_t,size_t> get_leaf_info(_in const dataset::dataset& train, _in const std::vector<size_t>& row_selections, _out leaf_info_t& leaf_info) {
		std::map<size_t,size_t> label_count;

		// get occurence
		for (const auto& row_index: row_selections) label_count[train.y.get_raw(row_index, 0).index]++;

		// get leaf_info from label_count.
		get_leaf_info(label_count, leaf_info);

		return label_count;
	}

	// after an early stop, there can be a leaf node have same label.
	// return
	//  - something combined : true
	//  - nothing   combined : false
	// ex)
	// salary <= 25373.075010
    //     age <= 23.000000: G1 (1/0)
    //     age > 23.000000: G1 (3/0)
	// ...
	// ==>
	// salary <= 25373.075010: G1 (4/0)
	// ...
	//
	static inline bool combine_same_label(_in _out tree_node& root) {
		bool found_something = false;
		bool found = false;

		// loop till there is no same label.
		for (int i=0; i<65536; i++) {
			found = false;
			// traverse all tree nodes.
			util::traverse_tree_node(root, [&found,&found_something](auto& treenode, int depth) -> bool {
				bool treenode_has_same_label = false;

				// finds if it has only leaf nodes that all refer to the same class.

				// skip root node.
				if (depth == 0) return true;

				// skip leaf node.
				if (treenode.leaf) return true;
				if (treenode.childs.size() == 0) THROW_GAENARI_INTERNAL_ERROR0;

				// check 'has only leaf nodes' and label index
				size_t label_string_index = treenode.childs[0]->leaf_info.label_string_index;
				treenode_has_same_label = true;
				for (const auto& child: treenode.childs) {
					if (not child->leaf) {
						treenode_has_same_label = false;
						break;
					}
					if (child->leaf_info.label_string_index != label_string_index) {
						treenode_has_same_label = false;
						break;
					}
				}

				if (not treenode_has_same_label) return true;

				// this treenode's children has same label.
				found = true;
				found_something = true;

				// remove child nodes and set as leaf node.
				treenode.leaf = true;
				engine::get_leaf_info(treenode.predicted_count.count, treenode.leaf_info);
				treenode.destroy_all_child_node(false);

				// traverse continue
				return true;
			});

			if (not found) break;
		}

		if (found) THROW_GAENARI_ERROR("too much cases for _combine_same_label.");
		return found_something;
	}

	// early stop if (split error rate * weight > parent error rate).
	//
	// -------+------------+-------------------+--------------
	// weight | early stop | pruning size      | tree size    
	// -------+------------+-------------------+--------------
	//     0  | no         | nothing           | massive         |
	// (0, 1) | yes        | a little          | apporopriate    | 
	//     1  | yes        | relatively        | apporopriate    |
	// (1,  ) | yes        | relatively        | small           V smaller tree, but not proportional decrease.
	// -------+------------+-------------------+--------------
	static inline bool check_early_stop_for_pruning(_in const tree_node& current, _in const std::vector<split_info>& split_infos, _in double early_stop_weight=0.0) {
		if (early_stop_weight == 0.0) return false;
		if (early_stop_weight < 0) THROW_GAENARI_INVALID_PARAMETER("invalid early_stop_weight.");
		leaf_info_t _leaf_info;
		size_t split_incorrect_count = 0;
		size_t split_total_count = 0;

		// get the current incorrect count.
		get_leaf_info(current.predicted_count.count, _leaf_info);
		size_t current_total_count = _leaf_info.incorrect_count + _leaf_info.correct_count;
		size_t current_incorrect_count = _leaf_info.incorrect_count;

		// get the incorrect count when splitting.
		for (const auto& split_info: split_infos) {
			get_leaf_info(split_info.predicted_count.count, _leaf_info);
			split_incorrect_count += _leaf_info.incorrect_count;
			split_total_count     += _leaf_info.correct_count + _leaf_info.incorrect_count;
		}
		if (split_total_count != current_total_count) THROW_GAENARI_ERROR("internal error(split_total_count != current_total_count).");

		// calc error rate of each cases.
		// - non split error rate
		// -     split error rate
		double current_error_rate = static_cast<double>(current_incorrect_count) / static_cast<double>(current_total_count);
		double split_error_rate   = static_cast<double>(split_incorrect_count)   / static_cast<double>(current_total_count);

		// multiply by weight.
		split_error_rate *= early_stop_weight;

		// when the error rate becomes larger when splitting, do early stop.
		if (current_error_rate <= split_error_rate) return true;

		// do not early stop. let's split.
		return false;
	}

	// post pruning weight
	//
	// post pruning conditions
	//  - terminal node(having only leaf nodes) only.
	//  - pruning if : terminal node error rate < sum of leaf node error rate * weight
	//  - repeat until no pruning occurs.
	// return
	//  -    pruning occured : true
	//  - no pruning occured : false
	// 
	// -------+--------------+-------------
	// weight | pruning size | tree size
	// -------+--------------+-------------
	// [,1.0] | not accepted | not accepted
	//   1.5  | 
	static inline bool post_pruning(_in _out tree_node& root, _in double weight) {
		// root traversing till no pruning found.
		bool found = false;
		if (weight < 1.0) return false;
		for (;;) {
			found = false;
			util::traverse_tree_node(root, [&found,&weight](tree_node& node, int depth) -> bool {
				bool do_pruning = false;

				// check only terminal node(=have only leaf nodes).
				if (not util::is_terminal_node(node)) return true;

				// get the current incorrect count.
				leaf_info_t _leaf_info;
				get_leaf_info(node.predicted_count.count, _leaf_info);
				size_t current_total_count = _leaf_info.incorrect_count + _leaf_info.correct_count;
				size_t current_incorrect_count = _leaf_info.incorrect_count;

				// get the incorrect count when splitting.
				size_t child_total_count = 0;
				size_t child_incorrect_count = 0;
				for (const auto& child: node.childs) {
					child_total_count += child->leaf_info.correct_count + child->leaf_info.incorrect_count;
					child_incorrect_count += child->leaf_info.incorrect_count;
				}

				if (current_total_count != child_total_count) THROW_GAENARI_ERROR("internal error(child_total_count != current_total_count.");

				// calc error rate of each cases.
				// - current node error rate
				// - child error rate
				double current_error_rate = static_cast<double>(current_incorrect_count) / static_cast<double>(current_total_count);
				double child_error_rate   = static_cast<double>(child_incorrect_count)   / static_cast<double>(child_total_count);

				// multiply by weight
				child_error_rate *= weight;

				// when the error rate becomes larger when splitting, do pruning.
				if (current_error_rate < child_error_rate) {
					do_pruning = true;
				}

				// pruning?
				if (do_pruning) {
					node.leaf = true;
					found = true;
					get_leaf_info(node.predicted_count.count, node.leaf_info);
					node.destroy_all_child_node(false);
				}

				return true;
			});
			if (not found) break;
		}
		return found;
	}

protected:
	// find the label index with the maximum occurence.
	// could a non-maximum be selected in the future?
	// since it's still greedy, it finds the max.
	// the processing of two or more same max follows the order of the map's key.
	static inline size_t get_label_index(_in const std::map<size_t,size_t>& label_count) {
		size_t r   = 0;
		size_t max = std::numeric_limits<size_t>::lowest();
		for (const auto i: label_count) {
			if (max < i.second) {
				max = i.second;
				r = i.first;
			}
		}
		return r;
	}

	static inline void get_leaf_info(_in const std::map<size_t,size_t>& label_count, _out leaf_info_t& leaf_info) {
		// clear
		leaf_info = method::decision_tree::leaf_info_t{};

		// get max occurence label index.
		leaf_info.label_string_index = get_label_index(label_count);

		// correct, incorrect count.
		for (const auto& index: label_count) {
			if (index.first == leaf_info.label_string_index) leaf_info.correct_count += index.second;
			else leaf_info.incorrect_count += index.second;
		}
	}

	// return parent information gain S
	template <typename count_t>
	static inline double calc_S(_in const dataset::dataset& train, _in const std::vector<size_t>& row_selections) {
		double S = 0.0;
		std::map<type::value_obj,count_t> label_count;
		
		// short alias name and intialize
		const auto& y = train.y;
		double size = static_cast<double>(row_selections.size());

		// count each label value, and calc 'S'
		for (const auto& row_index: row_selections) {
			const auto& label = y.get_raw(row_index, 0);
			label_count[label]++;
		}

		for (const auto& label: label_count) {
			auto p = static_cast<double>(label.second) / size;
			S -= p * std::log2(p);
		}

		return S;
	}

	// calc entropy.
	// _count_t is assumed to be either 'int' or 'size_t'. (size_t is more suitable for big data.)
	template<typename object_t, typename count_t>
	static inline double calc_entropy(_in const std::map<object_t, count_t>& label_count, _option_out double* value_count) {
		double _value_count = 0.0;
		double e = 0.0;

		// total count
		for (const auto& labels: label_count) _value_count += static_cast<double>(labels.second);

		// entropy calculation.
		for (const auto& labels: label_count) {
			if (labels.second == 0) continue;
			double p = static_cast<double>(labels.second) / _value_count;
			e -= p * std::log2(p);
		}

		// pass total count if required.
		if (value_count) *value_count = _value_count;

		return e;
	}

	// calc weighted entropy sum.
	// value_t is assumed to be either 'value_obj' or 'value'.
	// label_t is assumed to be size_t. (nominal label(strings index))
	// count_t is assumed to be either 'int' or 'size_t'. (size_t is more suitable for big data.)
	// if the total count has already been calculated, the counting process is passed.
	template <typename value_t, typename label_t, typename count_t>
	static inline double calc_weighted_entropy_sum(_in const std::map<value_t, std::map<label_t, count_t>>& value_label_count, _option_in size_t* total_count = nullptr) {
		double r = 0;
		double _total_count = 0;

		// get total count.
		if (total_count) {
			_total_count = static_cast<double>(*total_count);
		} else {
			size_t __total_count = 0;
			for (const auto& values: value_label_count)
				for (const auto& labels: values.second)
					__total_count += labels.second;
			_total_count = static_cast<double>(__total_count);
		}

		// for each values, and for each labels
		for (const auto& values: value_label_count) {
			double value_count = 0;
			auto e = calc_entropy(values.second, &value_count);
			double weight = value_count / _total_count;
			
			// E(S, feature) += count_value1_in_feature / size * E(each label count wehn feature == value) + ...
			// ------+------    ---------------+--------------   ---------------------+-------------------
			//       |                         |                                      |
			//       V                         V                                      V
			r                +=              weight            *                      e;
		}

		return r;
	}

	// typical ID3 method.
	static inline igr_result calc_igr_nominal_default_strategy(_in const dataset::dataset& train, _in const std::vector<size_t>& row_selections, _in size_t feature_index, _in double S) {
		igr_result r;
		double intrinsic_information_of_split = 0.0;
		std::map<type::value_obj, std::map<size_t, size_t>> value_label_count;

		// short alias name
		const auto& x = train.x;
		const auto& y = train.y;
		const auto& features = train.x.columns();

		// initialize
		auto total_count = row_selections.size();
		double size = static_cast<double>(total_count);

		// get data type
		auto data_type = features[feature_index].data_type;
		if (not ((data_type == dataset::data_type_t::data_type_string) or
			     (data_type == dataset::data_type_t::data_type_string_table))) THROW_GAENARI_INTERNAL_ERROR0;

		// counting
		for (const auto& row_index: row_selections) {
			const auto& value = x.get_raw(row_index, feature_index);
			const auto& label = y.get_raw(row_index, 0).index;

			// (value,label) count for information gain calculation.
			value_label_count
				[value]	// each value
				[label]	// each label
				++;		// count increment
		}

		// get weighted entropy sum.
		r.igr_value = calc_weighted_entropy_sum(value_label_count, &total_count);

		// calc information gain.
		// the higher the information gain, the better division.
		// if IG is 0, it means that there is no decrease in entropy by division.
		// the maximum IG(=E(S, feature) is 0) makes the best divide.
		// IG = S - E(S, feature)
		r.igr_value = S - r.igr_value;

		// current igr is the information gain.
		// calc intrinsic information gain to get information grain ratio.
		for (const auto& values: value_label_count) {
			// get value count
			double value_count = 0.0;
			for (const auto& labels: values.second) value_count += static_cast<double>(labels.second);
			double p = value_count / size;
			intrinsic_information_of_split -= p * std::log2(p);
		}

		// if intrinsic_information_of_split is zero, it means it has only one split.
		// stop criteria.
		if (intrinsic_information_of_split == 0) r.stop_criteria = true;

		// information_gain_ratio = information_gain / intrinsic_information_of_split
		r.igr_value /= intrinsic_information_of_split;

		// information_gain_ratio calculation completed,
		// then, get rule.

		// can get value distincts from value_label_count.
		// it's sorted by string index, and iterates in the order of appearance.
		for (const auto& distinct: value_label_count) {
			size_t string_index = distinct.first;
			rule_t rule;
			predicted_count_t predicted_count;
			rule.feature_indexes  = {feature_index};			// rule feature index : feature_index
			rule.type             = rule_t::rule_type::cmp_equ;	// rule type          : ==
			rule.args             = {string_index};				// rule args          : string_index
			predicted_count.count = distinct.second;
			r.split_infos.emplace_back(split_info(rule, predicted_count));
		}

		return r;
	}

	// split the numeric feature into two.
	static inline igr_result calc_igr_numeric_default_strategy(_in const dataset::dataset& train, _in const std::vector<size_t>& row_selections, _in size_t feature_index, _in double S) {
		igr_result r;
		size_t index = 0;
		size_t total_count = 0;
		double current_entropy = 0.0;
		double best_entropy = 0.0;
		double split_value = 0.0;
		size_t best_entropy_index = 0;
		double p = 0.0;
		type::value best_split_value;
		std::map<type::value,std::map<size_t,size_t>> value_label_count;	// using value instead of value_obj, map sorting is supported.
		std::map<rule_t::rule_type,std::map<size_t,size_t>> split;
		std::map<rule_t::rule_type,std::map<size_t,size_t>> best_split;
		std::map<size_t,size_t> acc, rest, total;

		// short alias name
		const auto& x = train.x;
		const auto& y = train.y;
		const auto& features = train.x.columns();

		// initialize
		double size = static_cast<double>(row_selections.size());

		// get data type
		auto data_type = features[feature_index].data_type;
		if (not common::is_numeric(data_type)) THROW_GAENARI_INTERNAL_ERROR0;

		// sorting value and count label count.
		for (const auto& row_index: row_selections) {
			// memory can be mimized(50%) by calling get_raw(),
			// but value was used because the key sort of std::map is guaranteed.
			const auto& value = x.get_value(row_index, feature_index);
			const auto& label = y.get_raw(row_index, 0).index;

			// (value,label) count for information gain calculation.
			// [value] is sorted by std::map.
			value_label_count
				[value]	// each value
				[label]	// each label
				++;		// count increment
		}

		// value_label_count is like this.
		// values(V1 ~ V4) are sorted.
		// L1, L2 is label and the number is count.
		// ---+------------------
		//    | V1   V2   V3   V4
		// ---+------------------
		// L1 |  1    0    1    1
		// L2 |  0    1    0    0

		// count total
		// ---+------------------    -----
		//    | V1   V2   V3   V4    TOTAL
		// ---+------------------ => -----
		// L1 |  1    0    1    1        3
		// L2 |  0    1    0    0        1
		for (const auto& count: value_label_count) {
			for (const auto& label: count.second) {
				total[label.first] += label.second;
			}
		}

		// find the index of the min entropy.
		// ready.
		best_entropy_index = 0;
		rest = total;
		best_entropy = std::numeric_limits<double>::max();
		acc.clear();
		best_split_value = value_label_count.begin()->first;
		best_split = {};
		index = 0;

		// the entropy of each value is calculated by the accumulated label count.
		//
		//     split_point ........................................
		//          |                                             :
		//  acc <-. | .-> rest                                    :
		//        | | |              .                            :
		//        | |-|- - - - - - > .                            :
		//        | | |    iterate   .                            :
		//        V V V              V                            :
		// ---+-----------------------                            :
		//    | V1      V2   V3   V4                              :
		// ---+-----------------------                            :
		for (const auto& split_point: value_label_count) { // <---'
			// update acc, rest label count
			for (const auto& label: split_point.second) {
				acc[label.first]  += label.second;
				rest[label.first] -= label.second;
			}

			// set split information and get it's entropy.
			split = {{rule_t::rule_type::cmp_lte, acc}, {rule_t::rule_type::cmp_gt, rest}};
			current_entropy = calc_weighted_entropy_sum(split);

			if (current_entropy < best_entropy) {
				// found better split point.
				best_entropy = current_entropy;
				best_entropy_index = index;
				best_split = split;
				best_split_value = split_point.first;
			}
			index++;
		}

		// calc information gain.
		// the higher the information gain, the better division.
		// if IG is 0, it means that there is no decrease in entropy by division.
		// the maximum IG(=E(S, feature) is 0) makes the best divide.
		// IG = S - E(S, feature)
		r.igr_value = S - best_entropy;

		// numeric feature does not divided by intrinsic information_of_split.
		// that is, the information gain ratio calculation process is not performed.
		// information gain == information gain ratio.

		// get rule.

		// split is two.
		if (2 != best_split.size()) THROW_GAENARI_INTERNAL_ERROR0;

		// first split.
		rule_t rule;
		predicted_count_t predicted_count;
		rule.feature_indexes  = {feature_index};				// rule feature index : feature_index
		rule.type             = best_split.begin()->first;		// rule type          : < or <= (lt or lte)
		rule.args	          = {best_split_value};				// rule args          : best split value
		predicted_count.count = best_split.begin()->second;		// statistics
		r.split_infos.emplace_back(split_info(rule, predicted_count));

		// second split.
		rule.feature_indexes  = {feature_index};				// rule feature index : feature_index
		rule.type             = (++best_split.begin())->first;	// rule type          : > or >= (gt or gte)
		rule.args	          = {best_split_value};				// rule args          : best split value
		predicted_count.count = (++best_split.begin())->second;	// statistics
		r.split_infos.emplace_back(split_info(rule, predicted_count));

		return r;
	}

	// information gain reference : https://planetcalc.com/8421/
	//                              https://medium.datadriveninvestor.com/decision-tree-algorithm-with-hands-on-example-e6c2afb40d38
	// information gain ratio     : https://www.ke.tu-darmstadt.de/lehre/archiv/ws0809/mldm/dt.pdf
	static inline igr_result calc_igr_main(_in const dataset::dataset& train, _in const std::vector<size_t>& row_selections, _in size_t feature_index, _in double S, _in split_strategy split_strategy) {
		double igr = 0.0;
		auto data_type = train.x.columns()[feature_index].data_type;
		
		if (split_strategy == split_strategy::split_strategy_default) {
			if (common::is_nominal(data_type))		return calc_igr_nominal_default_strategy(train, row_selections, feature_index, S);
			else if (common::is_numeric(data_type))	return calc_igr_numeric_default_strategy(train, row_selections, feature_index, S);
		}

		THROW_GAENARI_INTERNAL_ERROR0;
	}
}; // engine
} // decision_tree
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_ENGINE_HPP
